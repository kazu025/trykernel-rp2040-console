/*
 *** Try Kernel
 *      イベントフラグ
*/

#include <trykernel.h>
#include <knldef.h>

FLGCB   flgcb_tbl[CNF_MAX_FLGID];     /* イベントフラグ管理ブロック(FLGCB) */

/* イベントフラグの生成API */
ID tk_cre_flg( const T_CFLG *pk_cflg )
{
    ID      flgid;
    UINT    intsts;
    if(pk_cflg == NULL) return E_PAR;
    DI(intsts);     // 割込み禁止
    for(flgid = 0; flgid < CNF_MAX_FLGID &&  flgcb_tbl[flgid].state != KS_NONEXIST; flgid++);

    if(flgid < CNF_MAX_FLGID) {
        flgcb_tbl[flgid].state = KS_EXIST;
        flgcb_tbl[flgid].flgptn = pk_cflg->iflgptn;
        flgid++;
    } else {
        flgid = E_LIMIT;
    }
    EI(intsts);      // 割込み許可
    return flgid;
}

/* イベントフラグ待ちの条件チェック */
static BOOL check_flag(UINT flgptn, UINT waiptn, UINT wfmode)
{
    if(wfmode & TWF_ORW) {
        return ((flgptn & waiptn) != 0);
    } else {
        return ((flgptn &waiptn) == waiptn);
    }
}

/* イベントフラグのセットAPI */
ER tk_set_flg( ID flgid, UINT setptn )
{
    FLGCB   *flgcb;
    TCB     *tcb, *next;
    ER      err = E_OK;
    UINT    intsts;
    BOOL    need_schedule = FALSE;

    if(flgid <= 0 || flgid > CNF_MAX_FLGID) return E_ID;

    DI(intsts);     // 割込み禁止
    flgcb = &flgcb_tbl[--flgid];
    if(flgcb->state == KS_EXIST) {
        flgcb->flgptn |= setptn;            // フラグのセット

        for( tcb = wait_queue; tcb != NULL; tcb = next) {
            next = tcb->next;  // tqueue_add_entry()で tcb->next= NULLとしているため
            if((tcb->waifct == TWFCT_FLG) && (tcb->waiobj == flgid)) {
                if(check_flag(flgcb->flgptn, tcb->waiptn, tcb->wfmode)) {   // 条件成立の確認
                    tqueue_remove_entry( &wait_queue, tcb);                 // タスクをウェイトキューから外す
                    tcb->state	= TS_READY;
                    tcb->waifct	= TWFCT_NON;
                    *tcb->p_flgptn = flgcb->flgptn;
                    tqueue_add_entry( &ready_queue[PRI_INDEX(tcb->itskpri)], tcb);     // タスクをレディキューへつなぐ

                    /* READYタスクが発生したことを記憶 */
                    need_schedule = TRUE;

                    if ((tcb->wfmode & TWF_BITCLR) != 0 ) {
                        if ( (flgcb->flgptn &= ~(tcb->waiptn)) == 0 ) {     // 対象フラグのクリア
                            break;
                        }
                    }
                    if ((tcb->wfmode & TWF_CLR) != 0 ) {
                        flgcb->flgptn = 0;                                  // 全フラグのクリア
                        break;
                    }
                }
            }
        }
        if(need_schedule) {
            /* 待ちタスクの処理完了後、スケジューラを１回だけ実行 */
            scheduler();    // スケジューラを実行
        }
    } else {
        err = E_NOEXS;
    }

    EI(intsts);     // 割込み許可
    return err;
}

/* イベントフラグのクリアAPI */
ER tk_clr_flg( ID flgid, UINT clrptn )
{
    FLGCB   *flgcb;
    ER      err = E_OK;
    UINT    intsts;

    if(flgid <= 0 || flgid > CNF_MAX_FLGID) return E_ID;

    DI(intsts);     // 割込み禁止
    flgcb = &flgcb_tbl[--flgid];
    if(flgcb->state == KS_EXIST) {
        flgcb->flgptn &= clrptn;        // フラグのクリア
    } else {
        err = E_NOEXS;
    }
    EI(intsts);     // 割込み許可
    return err;
}

/* イベントフラグ待ちAPI */
ER tk_wai_flg( ID flgid, UINT waiptn, UINT wfmode, UINT *p_flgptn, TMO tmout )
{
    FLGCB   *flgcb;
    ER      err = E_OK;
    UINT    intsts;

    /* 割り込み・例外コンテキストからの待ちは禁止 */
    if(is_interrupt_context()) return E_CTX;

    if(flgid <= 0 || flgid > CNF_MAX_FLGID) return E_ID;
    if(p_flgptn == NULL || waiptn == 0) return E_PAR;

    DI(intsts);     // 割込み禁止
    flgcb = &flgcb_tbl[--flgid];
    if(flgcb->state == KS_EXIST) {
        if(check_flag(flgcb->flgptn, waiptn, wfmode)) {     // 待ち条件が成立している場合
            *p_flgptn = flgcb->flgptn;                      // 条件成立時のフラグ値を返す
            if ( (wfmode & TWF_BITCLR) != 0 ) {
                flgcb->flgptn &= ~waiptn;                   // 該当フラグのクリア
            }
            if ( (wfmode & TWF_CLR) != 0 ) {
                flgcb->flgptn = 0;                          // 全フラグのクリア
            }
        } else if(tmout == TMO_POL) {                       // 待ち条件不成立、かつ、待ち時間0の場合
            err = E_TMOUT;
        } else {                                            // 待ち条件不成立、待ち状態に移行
            tqueue_remove_top(&ready_queue[PRI_INDEX(cur_task->itskpri)]);     // タスクをレディキューから外す

            /* TCBの各種情報を変更する */
            cur_task->state     = TS_WAIT;      // タスクの状態を待ち状態に変更
            cur_task->waifct    = TWFCT_FLG;    // 待ち要因を設定
            cur_task->waiobj    = flgid;        // 待ちイベントフラグIDを設定
            cur_task->waitim    = ((tmout == TMO_FEVR)? tmout: tmout + TIMER_PERIOD);    // 待ち時間を設定
            cur_task->waiptn    = waiptn;
            cur_task->wfmode    = wfmode;
            cur_task->p_flgptn  = p_flgptn;
            cur_task->waierr    = &err;

            tqueue_add_entry(&wait_queue, cur_task);                // タスクをウェイトキューに繋ぐ
            scheduler();                                            // スケジューラを実行
        }
    } else {
        err = E_NOEXS;      // 未登録のイベントフラグ
    }

    EI(intsts);     // 割込み許可
    return err;
}
