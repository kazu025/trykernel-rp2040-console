#include <trykernel.h>
#include <knldef.h>
#include <response_probe.h>
#include "uart_tx.h"

#define LATENCY_WAIT_MS 50
#define LATENCY_SAMPLES 100U
static UW samples_us[LATENCY_SAMPLES];
static UW flash_runs[LATENCY_SAMPLES];
static UW flash_us[LATENCY_SAMPLES];
static UW flash_start_offset_us[LATENCY_SAMPLES];
static BOOL flash_inside[LATENCY_SAMPLES];

/* A: systimer_handlerのREADY登録直後、B: tk_dly_tskから復帰直後。 */
void task_latency(INT stacd, void *exinf)
{
    UINT intsts;
    UW i, min_us = 0xffffffffU, max_us = 0U;
    unsigned long long sum_us = 0;
    (void)stacd;
    (void)exinf;
    uart_tx_send("response: sampling 100 wakes, wait=50 ms, priority=7\r\n");
    DI(intsts);
    response_probe_task = cur_task;
    EI(intsts);
    for(i = 0; i < LATENCY_SAMPLES; i++) {
        UW resumed_us, ready_us;
        UW flash_sequence, ready_sequence, flash_start, flash_end;
        BOOL valid;
        ER err;
        DI(intsts);
        response_probe_valid = FALSE;
        EI(intsts);
        err = tk_dly_tsk(LATENCY_WAIT_MS);
        resumed_us = in_w(TIMER_TIMERAWL); /* B: ログや集計より先に読む。 */
        DI(intsts);
        ready_us = response_probe_ready_us;
        valid = response_probe_valid;
        ready_sequence = response_ready_flash_sequence;
        flash_sequence = response_flash_sequence;
        flash_start = response_flash_start_us;
        flash_end = response_flash_end_us;
        EI(intsts);
        if(err != E_OK || !valid) {
            DI(intsts);
            response_probe_task = NULL;
            EI(intsts);
            uart_tx_printf("response: invalid sample=%u er=%d stamp=%u\r\n",
                           i, err, (UINT)valid);
            tk_ext_tsk();
            for(;;) { }
        }
        /* 32 bitの符号なし減算。1回の待ちが約71.6分未満であること。 */
        samples_us[i] = resumed_us - ready_us;
        flash_runs[i] = flash_sequence - ready_sequence;
        flash_inside[i] = flash_runs[i] != 0U
            && (UW)(flash_start - ready_us) <= samples_us[i]
            && (UW)(flash_end - ready_us) <= samples_us[i];
        flash_us[i] = flash_inside[i] ? flash_end - flash_start : 0U;
        flash_start_offset_us[i] = flash_inside[i] ? flash_start - ready_us : 0U;
        if(samples_us[i] < min_us) min_us = samples_us[i];
        if(samples_us[i] > max_us) max_us = samples_us[i];
        sum_us += samples_us[i];
    }
    DI(intsts);
    response_probe_task = NULL;
    EI(intsts);
    uart_tx_printf("response: samples=%u min=%u us max=%u us avg=%u us\r\n",
                   LATENCY_SAMPLES, min_us, max_us, (UW)(sum_us / LATENCY_SAMPLES));
    for(i = 0; i < LATENCY_SAMPLES; i++) {
        uart_tx_printf("response[%u]=%u us flash_runs=%u inside=%u start=%u us scan=%u us\r\n",
                       i, samples_us[i], flash_runs[i], (UINT)flash_inside[i],
                       flash_start_offset_us[i], flash_us[i]);
        /* UARTキューのあふれを避ける。計測終了後なので結果に含まれない。 */
        tk_dly_tsk(10);
    }
    tk_ext_tsk();
    for(;;) { }
}
