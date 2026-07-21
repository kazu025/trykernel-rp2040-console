#ifndef UART_H
#define UART_H
/*
 * Uart driver for Try Kernel / RP2040 UART0
 * This driver provides simple polling-based uart send/receive functions.
 * Low-level register access is kept inside this driver.
 */
#include <trykernel.h>
void uart_init(void);
BOOL uart_can_send(void);
BOOL uart_can_recv(void);
void uart_putc(UB c);           // １バイト送信
UINT uart_puts(const char *s);  // 文字列送信
BOOL uart_getc_nonblock(UB *c);
UB uart_getc(void);
int uart_rx_getc(UB *data);     // リングバッファから１文字取り出す
void uart_rx_poll(void);            // ポーリングでリングバッファに格納
void uart_rxbuf_init(void);         // 受信バッファの初期化
UW uart_rx_overflow_count(void);    // 受信バッファのオーバーフロー回数を返す
UW uart_rx_hw_overrun_count(void);  // オーバーランカウントを返す
#endif /* UART_H */
