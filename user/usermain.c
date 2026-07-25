#include <trykernel.h>
#include "gpio.h"
#include "uart_sync.h"
#include "uart_tx.h"
#include "task_uartrx.h"
/*
 * タスク優先度:1〜16
 * 数字が小さいほど優先度が高い
 */
#define PRIORITY_LED        12
#define PRIORITY_UARTRX     4
#define PRIORITY_UARTTX     6
#define PRIORITY_LOG_A      8
#define PRIORITY_LOG_B      10
/*
 * stack size
 */
#define STACK_LED       1024
#define STACK_UARTRX    1024
#define STACK_UARTTX    1024
#define STACK_LOG_A     1024
#define STACK_LOG_B     1024

 /*
 * LED制御タスク1生成情報
 */
UW  tskstk_led1[STACK_LED/sizeof(UW)];   // LED制御タスク1のスタック
ID  tskid_led1;                     // LED制御タスク1のID番号
extern void task_led1(INT stacd, void *exinf);
T_CTSK  ctsk_led1 = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_led1,
    .itskpri    = PRIORITY_LED,
    .stksz      = STACK_LED,
    .bufptr     = tskstk_led1,
};

/* Uart受信タスク生成情報 */
UW  tskstk_uartrx[STACK_UARTRX/sizeof(UW)];   // Uart受信タスクのスタック
ID  tskid_uartrx;                     // Uart受信タスクのID番号
extern void task_uartrx(INT stacd, void *exinf);
T_CTSK  ctsk_uartrx = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_uartrx,
    .itskpri    = PRIORITY_UARTRX,
    .stksz      = STACK_UARTRX,
    .bufptr     = tskstk_uartrx,
};

/* Uart送信タスク生成情報 */
UW  tskstk_uarttx[STACK_UARTTX/sizeof(UW)];   // Uart送信タスクのスタック
ID  tskid_uarttx;                     // Uart送信タスクのID番号
extern void task_uarttx(INT stacd, void *exinf);
T_CTSK  ctsk_uarttx = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_uarttx,
    .itskpri    = PRIORITY_UARTTX,
    .stksz      = STACK_UARTTX,
    .bufptr     = tskstk_uarttx,
};


/* UartLog生成情報 */
UW  tskstk_uartlog_a[STACK_LOG_A/sizeof(UW)];
ID  tskid_uartlog_a;
extern void task_uartlog_a(INT stacd, void *exinf);
T_CTSK  ctsk_uartlog_a = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_uartlog_a,
    .itskpri    = PRIORITY_LOG_A,
    .stksz      = STACK_LOG_A,
    .bufptr     = tskstk_uartlog_a,
};

UW  tskstk_uartlog_b[STACK_LOG_B/sizeof(UW)];
ID  tskid_uartlog_b;
extern void task_uartlog_b(INT stacd, void *exinf);
T_CTSK  ctsk_uartlog_b = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_uartlog_b,
    .itskpri    = PRIORITY_LOG_B,
    .stksz      = STACK_LOG_B,
    .bufptr     = tskstk_uartlog_b,
};


int usermain(void)
{
    ER ercd;
    led25_init();   // PicoボードのLED初期化

    /*
     * UART 送信排他制御用セマフォの生成
     */
    ercd = uart_sync_init();
    if(ercd < E_OK){
        return (int)ercd;
    }
    ercd = uart_tx_init();
    if(ercd < E_OK){
        return (int)ercd;
    }
    ercd = task_uartrx_init();
    if(ercd < E_OK){
        return (int)ercd;
    }
    tk_dly_tsk(5);
    uart_tx_send("Hello Try Kernel\r\n");

    /* Uart送信タスク */
    tskid_uarttx = tk_cre_tsk(&ctsk_uarttx);
    if(tskid_uarttx < E_OK){
        return (int)tskid_uarttx;
    }
    tk_sta_tsk(tskid_uarttx, 0);

    /* LED制御タスク1の生成、実行 */
    tskid_led1 = tk_cre_tsk(&ctsk_led1);
    if(tskid_led1 < E_OK){
        return (int)tskid_led1;
    }
    tk_sta_tsk(tskid_led1, 0);

    /* Uart受信タスク */
    tskid_uartrx = tk_cre_tsk(&ctsk_uartrx);
    if(tskid_uartrx < E_OK){
        return (int)tskid_uartrx;
    }
    tk_sta_tsk(tskid_uartrx, 0);

    /* UART log task A */
    tskid_uartlog_a = tk_cre_tsk(&ctsk_uartlog_a);
    if(tskid_uartlog_a < E_OK){
        return (int)tskid_uartlog_a;
    }
    tk_sta_tsk(tskid_uartlog_a, 0);

    /* UART log task B */
    tskid_uartlog_b = tk_cre_tsk(&ctsk_uartlog_b);
    if(tskid_uartlog_b < E_OK){
        return (int)tskid_uartlog_b;
    }
    tk_sta_tsk(tskid_uartlog_b, 0);

    return 0;
}
