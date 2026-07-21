#include <trykernel.h>
#include "uart_sync.h"

/*
 * UART 送信排他制御用セマフォID
 */
static ID uart_sync_semid;
/*
 * UART 送信排他制御の初期化
 */
ER uart_sync_init(void){
    T_CSEM csem = {
        .sematr = TA_TFIFO | TA_FIRST,
        .isemcnt = 1,
        .maxsem  = 1,
    };
    uart_sync_semid = tk_cre_sem(&csem);
    if(uart_sync_semid < E_OK){
        return (ER)uart_sync_semid;
    }
    return E_OK;
}
/*
 * UART 送信権の取得
 */
ER uart_sync_lock(void){
    return tk_wai_sem(uart_sync_semid, 1, TMO_FEVR);
}
/*
 * UART 送信権の開放
 */
ER uart_sync_unlock(void){
    return tk_sig_sem(uart_sync_semid, 1);
}
