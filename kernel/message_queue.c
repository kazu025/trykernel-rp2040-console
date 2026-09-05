/*
 *** Try Kernel
 *      固定長メッセージキュー
 */

#include <trykernel.h>
#include <knldef.h>

MSGQCB msgqcb_tbl[CNF_MAX_MSGQID];

static void copy_message(void *dst, const void *src, INT size)
{
    UB *d = (UB *)dst;
    const UB *s = (const UB *)src;

    while(size-- > 0) {
        *d++ = *s++;
    }
}

static TCB *find_waiting_task(TWFCT waifct, ID msgqid)
{
    TCB *tcb;

    for(tcb = wait_queue; tcb != NULL; tcb = tcb->next) {
        if((tcb->waifct == waifct) && (tcb->waiobj == msgqid)) {
            return tcb;
        }
    }
    return NULL;
}

static void make_ready(TCB *tcb)
{
    tqueue_remove_entry(&wait_queue, tcb);
    tcb->state = TS_READY;
    tcb->waifct = TWFCT_NON;
    *tcb->waierr = E_OK;
    tqueue_add_entry(&ready_queue[PRI_INDEX(tcb->itskpri)], tcb);
}

static void enqueue_message(MSGQCB *msgqcb, const void *msg)
{
    UB *dst = msgqcb->buffer + (msgqcb->tail * msgqcb->msgsz);

    copy_message(dst, msg, msgqcb->msgsz);
    msgqcb->tail++;
    if(msgqcb->tail >= msgqcb->maxmsg) {
        msgqcb->tail = 0;
    }
    msgqcb->count++;
}

static void dequeue_message(MSGQCB *msgqcb, void *msg)
{
    UB *src = msgqcb->buffer + (msgqcb->head * msgqcb->msgsz);

    copy_message(msg, src, msgqcb->msgsz);
    msgqcb->head++;
    if(msgqcb->head >= msgqcb->maxmsg) {
        msgqcb->head = 0;
    }
    msgqcb->count--;
}

ID tk_cre_msgq(const T_CMSGQ *pk_cmsgq)
{
    ID msgqid;
    UINT intsts;

    if(pk_cmsgq == NULL) return E_PAR;
    if(pk_cmsgq->bufptr == NULL) return E_PAR;
    if((pk_cmsgq->msgsz <= 0) || (pk_cmsgq->maxmsg <= 0)) return E_PAR;
    if(pk_cmsgq->msgqatr != TA_TFIFO) return E_RSATR;

    DI(intsts);
    for(msgqid = 0;
        (msgqid < CNF_MAX_MSGQID) &&
        (msgqcb_tbl[msgqid].state != KS_NONEXIST);
        msgqid++);

    if(msgqid < CNF_MAX_MSGQID) {
        MSGQCB *msgqcb = &msgqcb_tbl[msgqid];
        msgqcb->state = KS_EXIST;
        msgqcb->buffer = (UB *)pk_cmsgq->bufptr;
        msgqcb->msgsz = pk_cmsgq->msgsz;
        msgqcb->maxmsg = pk_cmsgq->maxmsg;
        msgqcb->count = 0;
        msgqcb->head = 0;
        msgqcb->tail = 0;
        msgqid++;
    } else {
        msgqid = E_LIMIT;
    }
    EI(intsts);
    return msgqid;
}

ER tk_snd_msgq(ID msgqid, const void *msg, TMO tmout)
{
    MSGQCB *msgqcb;
    TCB *receiver;
    ER err = E_OK;
    UINT intsts;

    if(is_interrupt_context()) return E_CTX;
    if((msgqid <= 0) || (msgqid > CNF_MAX_MSGQID)) return E_ID;
    if((msg == NULL) || (tmout < TMO_FEVR)) return E_PAR;

    DI(intsts);
    msgqcb = &msgqcb_tbl[--msgqid];
    if(msgqcb->state != KS_EXIST) {
        err = E_NOEXS;
    } else {
        receiver = find_waiting_task(TWFCT_RCV_MSGQ, msgqid);
        if(receiver != NULL) {
            copy_message(receiver->rcvmsg, msg, msgqcb->msgsz);
            make_ready(receiver);
            scheduler();
        } else if(msgqcb->count < msgqcb->maxmsg) {
            enqueue_message(msgqcb, msg);
        } else if(tmout == TMO_POL) {
            err = E_TMOUT;
        } else {
            tqueue_remove_top(&ready_queue[PRI_INDEX(cur_task->itskpri)]);
            cur_task->state = TS_WAIT;
            cur_task->waifct = TWFCT_SND_MSGQ;
            cur_task->waiobj = msgqid;
            cur_task->waitim = (tmout == TMO_FEVR) ? tmout : tmout + TIMER_PERIOD;
            cur_task->sndmsg = msg;
            cur_task->waierr = &err;
            tqueue_add_entry(&wait_queue, cur_task);
            scheduler();
        }
    }
    EI(intsts);
    return err;
}

ER tk_rcv_msgq(ID msgqid, void *msg, TMO tmout)
{
    MSGQCB *msgqcb;
    TCB *sender;
    ER err = E_OK;
    UINT intsts;

    if(is_interrupt_context()) return E_CTX;
    if((msgqid <= 0) || (msgqid > CNF_MAX_MSGQID)) return E_ID;
    if((msg == NULL) || (tmout < TMO_FEVR)) return E_PAR;

    DI(intsts);
    msgqcb = &msgqcb_tbl[--msgqid];
    if(msgqcb->state != KS_EXIST) {
        err = E_NOEXS;
    } else if(msgqcb->count > 0) {
        dequeue_message(msgqcb, msg);

        /* 空いた1件へ、最初の送信待ちタスクのメッセージを格納する */
        sender = find_waiting_task(TWFCT_SND_MSGQ, msgqid);
        if(sender != NULL) {
            enqueue_message(msgqcb, sender->sndmsg);
            make_ready(sender);
            scheduler();
        }
    } else if(tmout == TMO_POL) {
        err = E_TMOUT;
    } else {
        tqueue_remove_top(&ready_queue[PRI_INDEX(cur_task->itskpri)]);
        cur_task->state = TS_WAIT;
        cur_task->waifct = TWFCT_RCV_MSGQ;
        cur_task->waiobj = msgqid;
        cur_task->waitim = (tmout == TMO_FEVR) ? tmout : tmout + TIMER_PERIOD;
        cur_task->rcvmsg = msg;
        cur_task->waierr = &err;
        tqueue_add_entry(&wait_queue, cur_task);
        scheduler();
    }
    EI(intsts);
    return err;
}
