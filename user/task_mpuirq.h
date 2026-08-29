#ifndef TASK_MPUIRQ_H
#define TASK_MPUIRQ_H

#include <trykernel.h>

ER task_mpuirq_init(void);
void task_mpuirq(INT stacd, void *exinf);
UW task_mpuirq_sample_count_get(void);
UW task_mpuirq_error_count_get(void);

#endif /* TASK_MPUIRQ_H */
