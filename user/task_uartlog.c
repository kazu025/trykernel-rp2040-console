#include <trykernel.h>

#include "uart_tx.h"
#define ENABLE_UART_LOG_TEST 0
void task_uartlog_a(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    UW count = 0;
    while (TRUE) {
#if ENABLE_UART_LOG_TEST
        uart_tx_printf("LOG A count: %u\r\n", count);
#endif
        count++;
        tk_dly_tsk(1900);
    }
}

void task_uartlog_b(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    UW count = 0;
    while (TRUE) {
#if ENABLE_UART_LOG_TEST
        uart_tx_printf("LOG B count: %u\r\n", count);
#endif
        count++;
         tk_dly_tsk(2300);
    }
}