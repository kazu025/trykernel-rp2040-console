#include <trykernel.h>
#include "gpio.h"
#include "uart.h"

/* LED制御タスク1生成情報 */
UW  tskstk_led1[1024/sizeof(UW)];   // LED制御タスク1のスタック
ID  tskid_led1;                     // LED制御タスク1のID番号
void task_led1(INT stacd, void *exinf);
T_CTSK  ctsk_led1 = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_led1,
    .itskpri    = 10,
    .stksz      = 1024,
    .bufptr     = tskstk_led1,
};

/* Uart受信タスク生成情報 */
UW  tskstk_uartrx[1024/sizeof(UW)];   // Uart受信タスクのスタック
ID  tskid_uartrx;                     // Uart受信タスクのID番号
extern void task_uartrx(INT stacd, void *exinf);
void task_uartrx(INT stacd, void *exinf);
T_CTSK  ctsk_uartrx = {
    .tskatr     = TA_HLNG | TA_RNG3 | TA_USERBUF,   // タスク属性
    .task       = task_uartrx,
    .itskpri    = 10,
    .stksz      = 1024,
    .bufptr     = tskstk_uartrx,
};

int usermain(void)
{
    led25_init();   // PicoボードのLED初期化
    uart_puts("Hello Try Kernel\r\n");
    /* LED制御タスク1の生成、実行 */
    tskid_led1 = tk_cre_tsk(&ctsk_led1);
    tk_sta_tsk(tskid_led1, 0);
    /* Uart受信タスク */
    tskid_uartrx = tk_cre_tsk(&ctsk_uartrx);
    tk_sta_tsk(tskid_uartrx, 0);
    return 0;
}
