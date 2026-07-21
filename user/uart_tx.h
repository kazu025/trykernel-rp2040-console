#ifndef UART_TX_H
#define UART_TX_H
/* -------------------------------------------------- */
#include <trykernel.h>

/*
 * UART送信キューの初期化
 */
ER uart_tx_init(void);
/*
 * 送信キューへ追加
 */
ER uart_tx_send(const char* text);
/*
 * 送信キューオーバーフローカウント
 */
UW uart_tx_get_overflow_count(void);
/*
 * フォーマット文字列をUART送信キューへ登録する
 */
ER uart_tx_printf(const char* format, ...);
/* -------------------------------------------------- */
#endif /* UART_TX_H */