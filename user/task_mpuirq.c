#include <trykernel.h>
#include "gpio.h"
#include "mpu6050.h"
#include "task_mpuirq.h"
#include "uart_tx.h"

#define MPU_INT_PIN          6U
#define MPU_EVENT_DATA_READY (1U << 0)

static ID mpu_flgid;
static volatile UW mpu_sample_count;
static volatile UW mpu_error_count;

static void mpu_data_ready_notify_from_isr(void)
{
    if(mpu_flgid > 0){
        (void)tk_set_flg(mpu_flgid, MPU_EVENT_DATA_READY);
    }
}

ER task_mpuirq_init(void)
{
    T_CFLG cflg;

    cflg.flgatr = TA_TFIFO;
    cflg.iflgptn = 0U;
    mpu_flgid = tk_cre_flg(&cflg);
    if(mpu_flgid < E_OK){
        return (ER)mpu_flgid;
    }

    mpu_sample_count = 0U;
    mpu_error_count = 0U;
    return E_OK;
}

UW task_mpuirq_sample_count_get(void)
{
    return mpu_sample_count;
}

UW task_mpuirq_error_count_get(void)
{
    return mpu_error_count;
}

void task_mpuirq(INT stacd, void *exinf)
{
    mpu6050_raw_data_t raw_data;
    UINT flgptn;
    UB int_status;

    (void)stacd;
    (void)exinf;

    if(mpu6050_init() == FALSE
            || mpu6050_enable_data_ready_interrupt() == FALSE
            || gpio_irq_init_rising(
                MPU_INT_PIN, mpu_data_ready_notify_from_isr) == FALSE){
        uart_tx_send("MPU data ready IRQ initialization error\r\n");
        return;
    }

    uart_tx_send("MPU data ready IRQ task start (GPIO6, 10Hz)\r\n");

    while(TRUE){
        if(tk_wai_flg(
                mpu_flgid,
                MPU_EVENT_DATA_READY,
                TWF_ORW | TWF_BITCLR,
                &flgptn,
                TMO_FEVR) < E_OK){
            mpu_error_count++;
            continue;
        }

        if(mpu6050_read_interrupt_status(&int_status) == FALSE
                || (int_status & 0x01U) == 0U
                || mpu6050_read_raw(&raw_data) == FALSE){
            mpu_error_count++;
            continue;
        }

        mpu_sample_count++;
    }
}
