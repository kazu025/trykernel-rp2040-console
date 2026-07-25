#include <trykernel.h>
#include "uart.h"
#include "console.h"
#include "uart_tx.h"
#include "task_uartrx.h"

#define UART_RX_EVENT_DATA (1U << 0)
static ID uart_rx_flgid;
/*
 * UART割り込みから呼ばれる受信通知関数
 */
static void uart_rx_notify_from_isr(void){
    if(uart_rx_flgid > 0){
        (void)tk_set_flg(uart_rx_flgid, UART_RX_EVENT_DATA);
    }
}
/*
 * UART受信タスク関連の初期化
 */
ER task_uartrx_init(void){
    T_CFLG cflg;
    cflg.flgatr = TA_TFIFO;
    cflg.iflgptn = 0U;

    uart_rx_flgid = tk_cre_flg(&cflg);
    if(uart_rx_flgid < E_OK){
        return (ER)uart_rx_flgid;
    }
    uart_rx_set_notify(uart_rx_notify_from_isr);
    return E_OK;
}
void task_uartrx(INT stacd, void *exinf)
{
    UB c;
    ER err;
    UINT flgptn;

    (void)stacd;
    (void)exinf;

    uart_rxbuf_init();
    console_init();
    uart_rx_irq_enable();

    uart_tx_send("\r\nuart rx task start\r\n");
    console_prompt();

    while (1) {
        /* UART割り込みから受信通知が来るまで待つ */
        err = tk_wai_flg(uart_rx_flgid, UART_RX_EVENT_DATA, TWF_ORW | TWF_BITCLR, &flgptn, TMO_FEVR);
        if(err < E_OK){
            continue;
        }
        while (uart_rx_getc(&c)) {
            console_input_char(c);
        }
    }
}
