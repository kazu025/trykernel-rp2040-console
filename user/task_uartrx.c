#include <trykernel.h>
#include "uart.h"
#include "console.h"
#include "uart_tx.h"
void task_uartrx(INT stacd, void *exinf)
{
    UB c;
    (void)stacd; (void)exinf;

    uart_rxbuf_init();
    console_init();
    uart_tx_send("\r\nuart rx task start\r\n");
    console_prompt();

    while (1) {
        /* Uart RX FIFO -> software ring buffer */
        uart_rx_poll();
        /* software ring buffer -> console input */
        while (uart_rx_getc(&c)) {
            console_input_char(c);
        }
        tk_dly_tsk(2);
    }
}
