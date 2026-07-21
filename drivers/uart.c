/*
 * Uart driver for Try Kernel / RP2040 URAT0
 * This driver provides simple polling-based uart send/receive functions.
 * Low-level register access is kept inside this driver.
 */
#include <trykernel.h>
#include <knldef.h>
#include "uart.h"

#define UART_BAUD_IBRD 67U
#define UART_BAUD_FBRD 52U  // (0.816 * 64) + 0.5 = 52.384
#define UART_LCRH_8N1_FIFO 0x70U // 8bit, no parity, 1 stop bit, FIFO enabled

/* --- UART RX buffer --- */
#define UART_RX_BUF_SIZE    128
#define UART_RX_BUF_MASK    (UART_RX_BUF_SIZE - 1)

static volatile UW uart_rx_head = 0;
static volatile UW uart_rx_tail = 0;
static volatile UW uart_rx_overflow = 0;
static volatile UW uart_rx_hw_overrun;
static UB uart_rx_buf[UART_RX_BUF_SIZE];

/* ---------------------------------------------------------- */
/* static func */
static int uart_rxbuf_put(UB data);
/*
 * UART初期化
 */
void uart_init(void){
    out_w(UART0_BASE + UARTx_IBRD, UART_BAUD_IBRD);                         /* ボーレート設定 */
    out_w(UART0_BASE + UARTx_FBRD, UART_BAUD_FBRD);
    out_w(UART0_BASE + UARTx_LCR_H, UART_LCRH_8N1_FIFO);                    /* データ形式設定 */
    out_w(UART0_BASE + UARTx_CR, UART_CR_RXE | UART_CR_TXE | UART_CR_EN);   /* 通信イネーブル */
}
/*
 * バッファ初期化関数
 */
void uart_rxbuf_init(void){
    uart_rx_head = 0;
    uart_rx_tail = 0;
    uart_rx_overflow = 0;
    uart_rx_hw_overrun = 0;
}
/*
 * 送信FiFo Full
 */
BOOL uart_can_send(void){
    return (in_w(UART0_BASE + UARTx_FR) & UART_FR_TXFF) == 0;
}
/*
 * 受信FiFo Empty
 */
BOOL uart_can_recv(void){
    return (in_w(UART0_BASE + UARTx_FR) & UART_FR_RXFE) == 0;
}
/*
 * １文字送信
 */
void uart_putc(UB c){
    while(!uart_can_send());
    out_w(UART0_BASE + UARTx_DR, ((UW)c) & 0xFFU);
}
/*
 * 文字列送信
 */
UINT uart_puts(const char *s){
    UINT count = 0;
    if(s == NULL) return 0;
    while(*s != '\0'){
        uart_putc((UB)(*s++));
        count++;
    }
    return count;
}
/*
 * 文字列受信：Non-ブロッキング
 */
BOOL uart_getc_nonblock(UB *c){
    if(c == NULL) return FALSE;
    if(uart_can_recv()){
        *c = (UB)(in_w(UART0_BASE + UARTx_DR) & 0xFFU);
        return TRUE;
    }
    return FALSE;
}
/*
 * 文字列受信
 */
UB uart_getc(void){
    UB c;
    while(!uart_getc_nonblock(&c));
    return c;
}
/*
 * オーバーフロー回数を取得
 */
UW uart_rx_overflow_count(void){
    return uart_rx_overflow;
}
UW uart_rx_hw_overrun_count(void){
    return uart_rx_hw_overrun;
}
/*
 * 受信バッファから１文字取り出す
*/
int uart_rx_getc(UB *data){
    if(data == NULL) return 0;
    if(uart_rx_head == uart_rx_tail){
        return 0; // buffer empty
    }
    *data = uart_rx_buf[uart_rx_tail];
    uart_rx_tail = (uart_rx_tail + 1) & UART_RX_BUF_MASK;
    return 1; // success
}
/*
 * Uart H/W Fifoからリングバッファへデータを転送
 */
void uart_rx_poll(void){
    UB data;
    UW status;

    status = in_w(UART0_BASE + UARTx_RSR_ECR);
    if((status & UART_RSR_OE) != 0){
        uart_rx_hw_overrun++;
        /* エラー状態をクリア */
        out_w(UART0_BASE + UARTx_RSR_ECR, 0U);
    }
    while(uart_can_recv()){
        data = (UB)(in_w(UART0_BASE + UARTx_DR) & 0xFFU);
        uart_rxbuf_put(data);
    }
}

/*
 * 受信バッファへ１文字入れる
 */
static int uart_rxbuf_put(UB data){
    UW next;
    next = (uart_rx_head + 1) & UART_RX_BUF_MASK;
    if(next == uart_rx_tail){
        uart_rx_overflow++;
        return 0; // buffer full
    }
    uart_rx_buf[uart_rx_head] = data;
    uart_rx_head = next;
    return 1; // success
}
