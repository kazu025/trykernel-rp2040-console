#ifndef TASK_MPUIRQ_H
#define TASK_MPUIRQ_H

#include <trykernel.h>

ER task_mpuirq_init(void);
void task_mpuirq(INT stacd, void *exinf);
UW task_mpuirq_sample_count_get(void);
UW task_mpuirq_error_count_get(void);
void task_mpuirq_motion_start(long long accel_magnitude_squared);
void task_mpuirq_motion_stop(void);
BOOL task_mpuirq_motion_is_enabled(void);

#endif /* TASK_MPUIRQ_H */
