#ifndef TASK_UARTRX_H
#define TASK_UARTRX_H
/* -------------------------------------------------- */
#include <trykernel.h>
ER task_uartrx_init(void);
void task_uartrx(INT stacd, void *exinf);
/* -------------------------------------------------- */
#endif /* TASK_UARTRX_H */