#include <trykernel.h>
#include "uart.h"
#include "command.h"
#include "console.h"

/* --- 1行入力バッファ --- */
#define CONSOLE_LINE_BUF_SIZE  128
static char console_line_buf[CONSOLE_LINE_BUF_SIZE];
static UW console_line_pos = 0;
/* --- static function --- */

/*
 * コンソール出力初期化
 */
void console_init(void){
    console_line_pos = 0;
    console_line_buf[0] = '\0';
}
/*
 * コマンド実行
 */
void console_input_char(UB ch){
    if(ch == '\r'){
        uart_puts("\r\n"); // Move to new line
        console_line_buf[console_line_pos] = '\0'; // Null-terminate the string
        command_execute(console_line_buf);
        
        console_line_pos = 0;
        console_line_buf[0] = '\0'; // Clear the line buffer

        console_prompt();
        return;
    }
    /* Backspace */
    if(ch == '\b' || ch == 0x7F){
        if(console_line_pos > 0){
            console_line_pos--;
            uart_puts("\b \b"); // Move cursor back, print space, move back again
        }
        return;
    }
    /* Regular character */
    if(console_line_pos < CONSOLE_LINE_BUF_SIZE - 1){
        console_line_buf[console_line_pos++] = (char)ch;
        uart_putc(ch);  /* Echo back */
    } else {
        // buffer full, ignore additional characters or handle overflow
        uart_putc('\a');  /* Bell character for buffer full */
    }
}

/*
 * プロンプト出力
 */
void console_prompt(void){
    uart_puts("> ");
}
