#include <trykernel.h>
#include "gpio.h"
#include "task_led.h"
#include "uart_tx.h"

typedef enum {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK
} LED_TASK_MODE;
static volatile LED_TASK_MODE led_task_mode = LED_MODE_OFF;
void led_task_set_on(void) {
    led_task_mode = LED_MODE_ON;
}
void led_task_set_off(void) {
    led_task_mode = LED_MODE_OFF;
}
void led_task_blink(void) {
    led_task_mode = LED_MODE_BLINK;
}
const char *led_task_mode_name(void) {
    switch (led_task_mode) {
        case LED_MODE_OFF:
            return "OFF";
        case LED_MODE_ON:
            return "ON";
        case LED_MODE_BLINK:
            return "BLINK";
        default:
            return "UNKNOWN";
    }
}
/* LED制御タスク1の実行関数 */
void task_led1(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    uart_tx_send("LED task start!!\r\n");
    while(1) {
        switch (led_task_mode) {
            case LED_MODE_OFF:
                led25_off();
                tk_dly_tsk(100);                // 0.1秒待ち
                break;
            case LED_MODE_ON:
                led25_on();
                tk_dly_tsk(100);                // 0.1秒待ち
                break;
            case LED_MODE_BLINK:
            default:
                led25_toggle();
                tk_dly_tsk(1000);                // 1秒待ち
                break;          
        }
    }
}
