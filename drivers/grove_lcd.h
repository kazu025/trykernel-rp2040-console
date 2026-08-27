#ifndef GROVE_LCD_H
#define GROVE_LCD_H

#include <trykernel.h>

#define GROVE_LCD_COLUMNS  16U
#define GROVE_LCD_ROWS      2U

BOOL grove_lcd_init(void);
BOOL grove_lcd_clear(void);
BOOL grove_lcd_set_cursor(UB column, UB row);
BOOL grove_lcd_write_text(const char *text);
BOOL grove_lcd_set_rgb(UB red, UB green, UB blue);

#endif /* GROVE_LCD_H */
