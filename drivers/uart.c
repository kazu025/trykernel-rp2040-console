/*
 * UART driver for TryKernel / RP2040 UART0
 * This driver provides polling-based transmission
 * and interrupt-driven reception.
 */
#include <trykernel.h>
#include <knldef.h>
#include "uart.h"

#define UART_BAUD_IBRD 67U
#define UART_BAUD_FBRD 52U  // (0.816 * 64) + 0.5 = 52.384
#define UART_LCRH_8N1_FIFO 0x70U // 8bit, no parity, 1 stop bit, FIFO enabled

/* --- UART RX buffer --- */
#define UART_RX_BUF_SIZE    128
#define UART_RX_BUF_MASK    (UART_RX_BUF_SIZE - 1)

static volatile UW uart_rx_head = 0;
static volatile UW uart_rx_tail = 0;
static volatile UW uart_rx_overflow = 0;
static volatile UW uart_rx_hw_overrun;
static volatile UB uart_rx_buf[UART_RX_BUF_SIZE];
static UART_RX_NOTIFY_FUNC uart_rx_notify;
static volatile UW uart_rx_irq_count;
static volatile UW uart_rt_irq_count;
/* ---------------------------------------------------------- */
/* static func */
static int uart_rxbuf_put(UB data);
/*
 * 割り込みカウンタ更新
 */
UW uart_rx_irq_count_get(void){
    return uart_rx_irq_count;
}
UW uart_rt_irq_count_get(void){
    return uart_rt_irq_count;
}
/*
 * UART初期化
 */
void uart_init(void){
    out_w(UART0_BASE + UARTx_IBRD, UART_BAUD_IBRD);                         /* ボーレート設定 */
    out_w(UART0_BASE + UARTx_FBRD, UART_BAUD_FBRD);
    out_w(UART0_BASE + UARTx_LCR_H, UART_LCRH_8N1_FIFO);                    /* データ形式設定 */
    out_w(UART0_BASE + UARTx_CR, UART_CR_RXE | UART_CR_TXE | UART_CR_EN);   /* 通信イネーブル */
}
/*
 * バッファ初期化関数
 */
void uart_rxbuf_init(void){
    uart_rx_head = 0;
    uart_rx_tail = 0;
    uart_rx_overflow = 0;
    uart_rx_hw_overrun = 0;
    uart_rx_irq_count = 0;
    uart_rt_irq_count = 0;
}
/*
 * UART0受信割り込みを有効化
 */
void uart_rx_irq_enable(void){
    UW intsts;
    DI(intsts);
    /* UART側の割り込みをすべてマスク */
    out_w(UART0_BASE + UARTx_IMSC, 0U);
    /* UART0の保留中割り込みをクリアする */
    out_w(UART0_BASE + UARTx_ICR,
        UART_ICR_RXIC | UART_ICR_RTIC | UART_ICR_OEIC);
    out_w(NVIC_ICPR, UART0_IRQ_MASK);
    /*
     * RXIM : RX FIFOがしきい値に達したとき
     * RTIM : RX FIFOにデータが残ったまま32ビット期間受信がないとき
     * OEIM : RX FIFOのオーバーランが発生したとき
     */
    out_w(UART0_BASE + UARTx_IMSC,
        UART_IMSC_RXIM | UART_IMSC_RTIM | UART_IMSC_OEIM);
    /* NVICでUART0 IRQ(IRQ20) を有効化する */
    out_w(NVIC_ISER, UART0_IRQ_MASK);
    EI(intsts);
}
/*
 * 送信FiFo Full
 */
BOOL uart_can_send(void){
    return (in_w(UART0_BASE + UARTx_FR) & UART_FR_TXFF) == 0;
}
/*
 * 受信FiFo Empty
 */
BOOL uart_can_recv(void){
    return (in_w(UART0_BASE + UARTx_FR) & UART_FR_RXFE) == 0;
}
/*
 * １文字送信
 */
void uart_putc(UB c){
    while(!uart_can_send());
    out_w(UART0_BASE + UARTx_DR, ((UW)c) & 0xFFU);
}
/*
 * 文字列送信
 */
UINT uart_puts(const char *s){
    UINT count = 0;
    if(s == NULL) return 0;
    while(*s != '\0'){
        uart_putc((UB)(*s++));
        count++;
    }
    return count;
}
/*
 * 文字列受信：Non-ブロッキング
 */
BOOL uart_getc_nonblock(UB *c){
    if(c == NULL) return FALSE;
    if(uart_can_recv()){
        *c = (UB)(in_w(UART0_BASE + UARTx_DR) & 0xFFU);
        return TRUE;
    }
    return FALSE;
}
/*
 * 文字列受信
 */
UB uart_getc(void){
    UB c;
    while(!uart_getc_nonblock(&c));
    return c;
}
/*
 * オーバーフロー回数を取得
 */
UW uart_rx_overflow_count(void){
    return uart_rx_overflow;
}
UW uart_rx_hw_overrun_count(void){
    return uart_rx_hw_overrun;
}
/*
 * 受信バッファから１文字取り出す
*/
int uart_rx_getc(UB *data){
    if(data == NULL) return 0;
    if(uart_rx_head == uart_rx_tail){
        return 0; // buffer empty
    }
    *data = uart_rx_buf[uart_rx_tail];
    uart_rx_tail = (uart_rx_tail + 1) & UART_RX_BUF_MASK;
    return 1; // success
}
/*
 * UART0受信割り込みハンドラ
 * UART0割り込み発生時にH/W FIFOから
 * ソフトウェアリングバッファへデータを転送する。
 *
 * コンソール処理は行わない。
 * データ格納後、登録された通知関数を
 * 割り込みコンテキストから呼び出す。
 */
void uart0_irq_handler(void){
    UB data;    // 受信データ/
    UW mis;     // 割り込み要因
    UW status;  // 受信エラー状態
    UW clear = 0U; // クリアする割り込み要因
    BOOL received = FALSE;

    mis = in_w(UART0_BASE + UARTx_MIS); // Masked Interrupt Status Register
    status = in_w(UART0_BASE + UARTx_RSR_ECR); // Receive Status Register
    /* HW overrun　検出 */
    if(((mis & UART_MIS_OEMIS) != 0U) || ((status & UART_RSR_OE) != 0U)){
        uart_rx_hw_overrun++;
        /* error clear */
        out_w(UART0_BASE + UARTx_RSR_ECR, 0U);
        clear |= UART_ICR_OEIC;
    }
    /* FIFO全データ読み出し */
    while(uart_can_recv()){
        data = (UB)(in_w(UART0_BASE + UARTx_DR) & 0xFFU);
        if(uart_rxbuf_put(data)){
            received = TRUE;
        }
    }
    if((mis & UART_MIS_RXMIS) != 0U){
        uart_rx_irq_count++;
        /* RX FIFO 割り込み要因クリア */
        clear |= UART_ICR_RXIC;
    }
    if((mis & UART_MIS_RTMIS) != 0U){
        uart_rt_irq_count++;
        /* 受信タイムアウト割り込み要因クリア */
        clear |= UART_ICR_RTIC;
    }
    if(clear != 0U){
        /* 割り込み要因をクリア */
        out_w(UART0_BASE + UARTx_ICR, clear);
    }
    /*
     * 割り込みコンテキストから通知する
     * 登録する関数では待ち処理などを行わないこと
     */
    if((received != FALSE) && (uart_rx_notify != NULL)){
        uart_rx_notify();
    }
}
/*
 * 受信バッファへ１文字入れる
 */
static int uart_rxbuf_put(UB data){
    UW next;
    next = (uart_rx_head + 1) & UART_RX_BUF_MASK;
    if(next == uart_rx_tail){
        uart_rx_overflow++;
        return 0; // buffer full
    }
    uart_rx_buf[uart_rx_head] = data;
    uart_rx_head = next;
    return 1; // success
}

/*
 * 通知関数の登録処理
 */
void uart_rx_set_notify(UART_RX_NOTIFY_FUNC notify){
    UW intsts;
    DI(intsts);
    uart_rx_notify = notify;
    EI(intsts);
}

