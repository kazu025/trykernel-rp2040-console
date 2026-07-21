#include <trykernel.h>

#include "uart_tx.h"
#include "uart_tx_internal.h"
#include "mini_printf.h"
/*
 * UART送信キュー設定　登録最大件数
 */
#define UART_TX_QUEUE_SIZE      16U

/*
 * UART送信イベント
 */
#define UART_TX_EVENT_DATA  (1U << 0)

typedef struct {
    /* メッセージサイズ（固定長） */
    char text[UART_TX_MESSAGE_SIZE];
} UART_TX_MESSAGE;

/*
 * UART送信キュー
 */
static UART_TX_MESSAGE uart_tx_queue[UART_TX_QUEUE_SIZE];

/*
 * キュー管理変数
 */
static UW uart_tx_head;     // 次に書き込む位置
static UW uart_tx_tail;     // 次に読み出す位置
static UW uart_tx_count;    // 現在登録されている件数

/*
 * キュー保護用セマフォ
 */
static ID uart_tx_semid;
/* static 変数*/
static volatile UW uart_tx_overflow_count; // 送信キューオーバーフローカウント
/*
 * UART送信タスク通知用イベントフラグ
 */
static ID uart_tx_flgid;

/* 内部関数 */
static ER uart_tx_queue_push(const char *text);
static ER uart_tx_queue_pop(char *text);
static void uart_tx_copy_string(
    char *dst,
    const char *src,
    UW size
);

/*
 * UART送信機能の初期化
 */
ER uart_tx_init(void)
{
    T_CSEM csem;
    T_CFLG cflg;
    /*
     * セマフォ設定（バイナリセマフォ）
     * 初期カウント:1
     * 最大カウント:1
     */
    csem.sematr  = TA_TFIFO | TA_FIRST;
    csem.isemcnt = 1;
    csem.maxsem  = 1;
    /*
     * イベントフラグ設定
     * 初期イベントバターン:0 (データなし)
     */
    cflg.iflgptn = 0U;
    cflg.flgatr = TA_TFIFO;
    /*
     * リングキュー設定(データなし)
     */
    uart_tx_head  = 0;
    uart_tx_tail  = 0;
    uart_tx_count = 0;
    /* overflowカウント初期化 */

    uart_tx_overflow_count = 0;
    /*
     * キュー保護用セマフォを生成
     */
    uart_tx_semid = tk_cre_sem(&csem);
    if (uart_tx_semid < E_OK) {
        return (ER)uart_tx_semid;
    }

    /*
     * データ到着通知用イベントフラグを生成
     */
    uart_tx_flgid = tk_cre_flg(&cflg);
    if (uart_tx_flgid < E_OK) {
        return (ER)uart_tx_flgid;
    }

    return E_OK;
}

/*
 * UART送信キューへ文字列を登録
 */
ER uart_tx_send(const char *text)
{
    ER err;
    ER unlock_err;

    if (text == NULL) {
        return E_PAR;
    }

    /*
     * キューをロック:セマフォ取得(無限待ち)
     */
    err = tk_wai_sem(uart_tx_semid, 1, TMO_FEVR);
    if (err < E_OK) {
        return err;
    }
    /*
     * 文字列をリングキューに追加する
     */
    err = uart_tx_queue_push(text);

    /*
     * pushの成否に関係なくロックを解除する
     */
    unlock_err = tk_sig_sem(uart_tx_semid, 1);

    if (err < E_OK) {
        return err;
    }

    if (unlock_err < E_OK) {
        return unlock_err;
    }

    /*
     * UART送信タスクへデータ到着を通知
     */
    return tk_set_flg(
        uart_tx_flgid,
        UART_TX_EVENT_DATA
    );
}
/*
 * 送信キューオーバーフローカウント取得
 */
UW uart_tx_get_overflow_count(void){
    return uart_tx_overflow_count;
}
/*
 * Uart送信データの登録通知を待つ
 */
ER uart_tx_wait(void){
    UINT flgptn;
    return tk_wai_flg(uart_tx_flgid,
        UART_TX_EVENT_DATA,     // データ到着ビット
        TWF_ORW | TWF_BITCLR,   // 指定ビットのどれかが立てば解除、ビットクリア
        &flgptn,
        TMO_FEVR);              // イベントが来るまで無限待ち
}
/*
 * 文字列を送信キューに登録
 */
ER uart_tx_printf(const char *format, ...){
    char text[UART_TX_MESSAGE_SIZE];
    va_list args;
    INT length;

    if(format == NULL){
        return E_PAR;
    }
    va_start(args, format);
    length = mini_vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if(length < 0){
        return (ER)length;
    }
    return uart_tx_send(text);
}
/*
 * キューをロックして1件取得
 */
ER uart_tx_receive(char *text)
{
    ER err;
    ER unlock_err;
    if(text == NULL){
        return E_PAR;
    }
    /* セマフォ取得 */
    err = tk_wai_sem(uart_tx_semid, 1, TMO_FEVR);
    if (err < E_OK) {
        return err;
    }
    /* キューから一件取得 */
    err = uart_tx_queue_pop(text);
    /* 開放 */
    unlock_err = tk_sig_sem(uart_tx_semid, 1);

    if (err < E_OK) {
        return err;
    }

    return unlock_err;
}


/* -------------------------------------------------- */
/*
 * UART送信キューへ1件追加
 *
 * 呼び出し側でセマフォを取得していること。
 */
static ER uart_tx_queue_push(const char *text)
{
    if (uart_tx_count >= UART_TX_QUEUE_SIZE) {
        uart_tx_overflow_count++;
        return E_QOVR;
    }

    uart_tx_copy_string(
        uart_tx_queue[uart_tx_head].text,
        text,
        UART_TX_MESSAGE_SIZE
    );

    uart_tx_head++;

    if (uart_tx_head >= UART_TX_QUEUE_SIZE) {
        uart_tx_head = 0;
    }

    uart_tx_count++;

    return E_OK;
}

/*
 * UART送信キューから1件取得
 *
 * 呼び出し側でセマフォを取得していること。
 */
static ER uart_tx_queue_pop(char *text)
{
    if (text == NULL) {
        return E_PAR;
    }

    if (uart_tx_count == 0) { // キーが空
        return E_TMOUT;
    }
    /* 現在のtailから文字列を読み出す */
    uart_tx_copy_string(
        text,
        uart_tx_queue[uart_tx_tail].text,
        UART_TX_MESSAGE_SIZE
    );
    /* tailを進める */
    uart_tx_tail++;
    if (uart_tx_tail >= UART_TX_QUEUE_SIZE) {
        uart_tx_tail = 0;
    }
    /* 登録件数を減らす*/
    uart_tx_count--;

    return E_OK;
}


/*
 * 最大sizeバイトの領域へ文字列をコピー
 */
static void uart_tx_copy_string(char *dst, const char *src, UW size){
    UW i;

    if ((size == 0U) ||
        (dst == NULL) ||
        (src == NULL)) {
        return;
    }

    for (i = 0; i < (size - 1U); i++) {
        if (src[i] == '\0') {
            break;
        }

        dst[i] = src[i];
    }

    dst[i] = '\0';
}
