#ifndef UART_SYNC_H
#define UART_SYNC_H
/* ---------------------------------------------------------------- */
#include <trykernel.h>
/*
 * UART 送信排他制御の初期化
 */
ER uart_sync_init(void);
/*
 * UART 送信権の取得
 */
ER uart_sync_lock(void);
/*
 * UART 送信権の開放
 */
ER uart_sync_unlock(void);
/* ---------------------------------------------------------------- */
#endif /* UART_SYNC_H */