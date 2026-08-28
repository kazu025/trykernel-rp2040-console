#include <trykernel.h>

#include "adt7410.h"
#include "grove_lcd.h"
#include "task_lcdtemp.h"
#include "uart_tx.h"

#define LCDTEMP_UPDATE_PERIOD  1000

static void format_temperature_line(INT temperature_milli_c, char *line)
{
    UINT magnitude;
    UINT integer_part;
    UINT fractional_part;
    UINT divisor;
    UW pos = 0U;

    for(UW i = 0U; i < GROVE_LCD_COLUMNS; i++){
        line[i] = ' ';
    }
    line[GROVE_LCD_COLUMNS] = '\0';

    if(temperature_milli_c < 0){
        line[pos++] = '-';
        magnitude = (UINT)(-temperature_milli_c);
    }else{
        magnitude = (UINT)temperature_milli_c;
    }

    integer_part = magnitude / 1000U;
    fractional_part = magnitude % 1000U;

    divisor = 1U;
    while((integer_part / divisor) >= 10U){
        divisor *= 10U;
    }

    do{
        line[pos++] = (char)('0' + (integer_part / divisor));
        integer_part %= divisor;
        divisor /= 10U;
    }while(divisor != 0U);

    line[pos++] = '.';
    line[pos++] = (char)('0' + (fractional_part / 100U));
    line[pos++] = (char)('0' + ((fractional_part / 10U) % 10U));
    line[pos++] = (char)('0' + (fractional_part % 10U));
    line[pos++] = ' ';
    line[pos] = 'C';
}

void task_lcdtemp(INT stacd, void *exinf)
{
    INT temperature_milli_c;
    char temperature_line[GROVE_LCD_COLUMNS + 1U];
    BOOL initialized = FALSE;

    (void)stacd;
    (void)exinf;

    while(TRUE){
        if(initialized == FALSE){
            if(grove_lcd_init() == FALSE
                    || grove_lcd_set_cursor(0U, 0U) == FALSE
                    || grove_lcd_write_text("ADT7410 Temp    ") == FALSE){
                uart_tx_send("LCD temperature task initialization error\r\n");
                tk_dly_tsk(LCDTEMP_UPDATE_PERIOD);
                continue;
            }
            initialized = TRUE;
        }

        if(adt7410_read_temperature(&temperature_milli_c) == FALSE){
            uart_tx_send("LCD temperature task sensor error\r\n");
        }else{
            format_temperature_line(
                temperature_milli_c,
                temperature_line
            );

            if(grove_lcd_set_cursor(0U, 1U) == FALSE
                    || grove_lcd_write_text(temperature_line) == FALSE){
                uart_tx_send("LCD temperature task write error\r\n");
            }
        }

        tk_dly_tsk(LCDTEMP_UPDATE_PERIOD);
    }
}
