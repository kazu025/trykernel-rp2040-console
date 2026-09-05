#ifndef TASK_MOTIONLED_H
#define TASK_MOTIONLED_H

#include <trykernel.h>

ER task_motionled_init(void);
void task_motionled(INT stacd, void *exinf);
void task_motionled_set_off(void);
void task_motionled_set_settling(void);
void task_motionled_set_moving(void);
void task_motionled_set_still(void);

#endif /* TASK_MOTIONLED_H */
