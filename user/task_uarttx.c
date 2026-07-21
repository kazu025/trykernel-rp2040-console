#include <trykernel.h>
#include "uart_tx_internal.h"
#include "uart.h"
#include "uart_sync.h"
/*
 * UART送信専用タスク
 */
void task_uarttx(INT stacd, void *exinf)
{
    char text[UART_TX_MESSAGE_SIZE];
    ER err;

    (void)stacd;
    (void)exinf;
    err = uart_sync_lock();
    if(err>=E_OK){
        uart_puts("\r\nuart tx task start\r\n");
        uart_sync_unlock();
    }
    while (TRUE) {
        /* 送信要求が登録されるまで待つ */
        err = uart_tx_wait();
        if (err < E_OK) {
            continue;
        }
        /* キューに入っているメッセージを
         * 空になるまですべて送信する。*/
        while (uart_tx_receive(text) == E_OK) {
            err = uart_sync_lock();
            if (err >= E_OK) {
                uart_puts(text);
                uart_sync_unlock();
            }
        }
    }
}