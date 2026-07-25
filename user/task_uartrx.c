#include <trykernel.h>
#include "uart.h"
#include "console.h"
#include "uart_tx.h"
void task_uartrx(INT stacd, void *exinf)
{
    UB c;
    (void)stacd; (void)exinf;

    uart_rxbuf_init();
    uart_rx_irq_enable();
    console_init();
    uart_tx_send("\r\nuart rx task start\r\n");
    console_prompt();

    while (1) {
        /* UART0 IRQが格納したデータをコンソール入力へ渡す */
        while (uart_rx_getc(&c)) {
            console_input_char(c);
        }
        tk_dly_tsk(2);
    }
}
