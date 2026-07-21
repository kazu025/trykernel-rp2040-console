#ifndef UART_TX_INTERNAL_H
#define UART_TX_INTERNAL_H
/* -------------------------------------------------- */
#include <trykernel.h>
/* 送信メッセージ長 */
#define UART_TX_MESSAGE_SIZE  128U

/* 非公開関数 */
ER uart_tx_wait(void);
ER uart_tx_receive(char *tex);
/* -------------------------------------------------- */
#endif /* UART_TX_INTERNAL_H */