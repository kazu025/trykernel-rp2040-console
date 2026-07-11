#ifndef TASK_LED_H
#define TASK_LED_H

#include <trykernel.h>

void task_led1(INT stacd, void *exinf);

void led_task_set_on(void);
void led_task_set_off(void);
void led_task_blink(void);
const char *led_task_mode_name(void);

#endif /* TASK_LED_H */