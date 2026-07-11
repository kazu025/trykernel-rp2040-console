#ifndef CONSOLE_H
#define CONSOLE_H
/* ---------------------------------------------- */
#include <trykernel.h>

void console_init(void);
void console_input_char(UB ch);
void console_prompt(void);

/* ---------------------------------------------- */
#endif /* CONSOLE_H */