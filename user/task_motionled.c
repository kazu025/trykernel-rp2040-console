#include <trykernel.h>
#include "gpio.h"
#include "task_motionled.h"
#include "uart_tx.h"

#define MOTION_LED_PIN              16U
#define MOTION_LED_EVENT_CHANGED    (1U << 0)
#define MOTION_LED_SETTLING_PERIOD  300
#define MOTION_LED_STILL_PERIOD     1000

typedef enum {
    MOTION_LED_MODE_OFF,
    MOTION_LED_MODE_SETTLING,
    MOTION_LED_MODE_MOVING,
    MOTION_LED_MODE_STILL
} MOTION_LED_MODE;

static ID motion_led_flgid;
static volatile MOTION_LED_MODE motion_led_mode;

static void motion_led_set_mode(MOTION_LED_MODE mode)
{
    motion_led_mode = mode;
    if(motion_led_flgid > 0){
        (void)tk_set_flg(motion_led_flgid, MOTION_LED_EVENT_CHANGED);
    }
}

ER task_motionled_init(void)
{
    T_CFLG cflg;

    gpio_init_out(MOTION_LED_PIN);
    gpio_clear(MOTION_LED_PIN);
    motion_led_mode = MOTION_LED_MODE_OFF;

    cflg.flgatr = TA_TFIFO;
    cflg.iflgptn = 0U;
    motion_led_flgid = tk_cre_flg(&cflg);
    if(motion_led_flgid < E_OK){
        return (ER)motion_led_flgid;
    }
    return E_OK;
}

void task_motionled_set_off(void)
{
    motion_led_set_mode(MOTION_LED_MODE_OFF);
}

void task_motionled_set_settling(void)
{
    motion_led_set_mode(MOTION_LED_MODE_SETTLING);
}

void task_motionled_set_moving(void)
{
    motion_led_set_mode(MOTION_LED_MODE_MOVING);
}

void task_motionled_set_still(void)
{
    motion_led_set_mode(MOTION_LED_MODE_STILL);
}

void task_motionled(INT stacd, void *exinf)
{
    MOTION_LED_MODE applied_mode = MOTION_LED_MODE_OFF;
    UINT flgptn;
    TMO timeout;
    ER err;

    (void)stacd;
    (void)exinf;

    gpio_clear(MOTION_LED_PIN);
    uart_tx_send("Motion LED task start (GPIO16)\r\n");

    while(TRUE){
        if(motion_led_mode != applied_mode){
            applied_mode = motion_led_mode;
            if(applied_mode == MOTION_LED_MODE_OFF){
                gpio_clear(MOTION_LED_PIN);
            }else{
                /* 点滅モードはHighから開始し、移動中は点灯を維持する */
                gpio_set(MOTION_LED_PIN);
            }
        }

        if(applied_mode == MOTION_LED_MODE_SETTLING){
            timeout = MOTION_LED_SETTLING_PERIOD;
        }else if(applied_mode == MOTION_LED_MODE_STILL){
            timeout = MOTION_LED_STILL_PERIOD;
        }else{
            timeout = TMO_FEVR;
        }

        err = tk_wai_flg(
            motion_led_flgid,
            MOTION_LED_EVENT_CHANGED,
            TWF_ORW | TWF_BITCLR,
            &flgptn,
            timeout
        );

        if((err == E_TMOUT)
                && ((applied_mode == MOTION_LED_MODE_SETTLING)
                    || (applied_mode == MOTION_LED_MODE_STILL))){
            gpio_toggle(MOTION_LED_PIN);
        }
    }
}
