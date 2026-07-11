#include <trykernel.h>
#include "uart.h"
#include "task_led.h"
#include "command.h"


/* --- コマンドバッファ最大数 --- */
#define CMD_MAX_ARGS    8
static void cmd_help(int argc, char *argv[]);
static void cmd_status(int argc, char *argv[]);
static void cmd_echo(int argc, char *argv[]);
static void cmd_led(int argc, char *argv[]);
static int split_args(char *line, char *argv[], int max_args);
static BOOL str_eq(const char *a, const char *b);
typedef void (*cmd_func_t)(int argc, char *argv[]);
typedef struct {
    const char *name;
    cmd_func_t func;
    const char *help;
} command_t;
static const command_t command_table[] = {
    {"help",    cmd_help,   "show command list"},
    {"status",  cmd_status, "show system status"},
    {"echo",    cmd_echo,  "echo arguments"},
    {"led",     cmd_led,    "led on/off/blink"}
};
static const int command_count = sizeof(command_table)/sizeof(command_table[0]);

/* 受信した行を処理する */
void command_execute(char *line){
    char *argv[CMD_MAX_ARGS];
    int argc;
    argc = split_args(line, argv, CMD_MAX_ARGS);
    if(argc == 0) return;
    for(int i=0; i<command_count; i++){
        if(str_eq(argv[0], command_table[i].name) == TRUE){
            command_table[i].func(argc, argv);
            return;
        }
    }
    uart_puts("unknown command: ");
    uart_puts(argv[0]);
    uart_puts("\r\n");
}



/*
 * 文字列比較
 */
static BOOL str_eq(const char *a, const char *b)
{
    while ((*a != '\0') && (*b != '\0')) {
        if (*a != *b) {
            return FALSE;
        }

        a++;
        b++;
    }

    return ((*a == '\0') && (*b == '\0'));
}
/*
 * トークン分割
 */
static int split_args(char *line, char *argv[], int max_args){
    int argc = 0;
    char *p = line;
    while(*p != '\0'){
        while(*p == ' ' || *p == '\t')  p++;
        if(*p == '\0')  break;
        if(argc >= max_args) break;
        argv[argc++] = p;
        while(*p != '\0' && *p != ' ' && *p != '\t')  p++;
        if(*p == '\0')  break;
        *p = '\0';
        p++;
    }
    return argc;
}
/*
 * コマンド:Help (Help表示)
 */
static void cmd_help(int argc, char *argv[]){
    (void)argc;
    (void)argv;
    uart_puts("commands:\r\n");
    for(int i=0; i<command_count; i++){
        uart_puts("   ");
        uart_puts(command_table[i].name);
        uart_puts(" - ");
        uart_puts(command_table[i].help);
        uart_puts("\r\n");
    }   
}
/*
 * コマンド:status　(ステータス表示)
 */
static void cmd_status(int argc, char *argv[]){
   (void)argc; (void)argv;
   uart_puts("TryKernel status: running\r\n"); 
   uart_puts("LED mode: ");
   uart_puts(led_task_mode_name());
   uart_puts("\r\n");
}
/*
 * コマンド:Echo (Echo 表示)
 */
static void cmd_echo(int argc, char *argv[]){
    for(int i=1; i<argc; i++){
        uart_puts(argv[i]);
        if(i != argc - 1)
            uart_puts(" ");
    }
    uart_puts("\r\n");
}
/*
 * コマンド:LED (LED表示)
 */
static void cmd_led(int argc, char *argv[]){
    if (argc < 2){
        uart_puts("usage: led on|off|blink\r\n");
        return ;
    }else if (str_eq(argv[1], "on") == TRUE){
        led_task_set_on();
        uart_puts("led on\r\n");
    }else if (str_eq(argv[1], "off") == TRUE){
        led_task_set_off();
        uart_puts("led off\r\n");
    }else if (str_eq(argv[1], "blink") == TRUE){
        led_task_blink();
        uart_puts("led blink\r\n");
    }else{
        uart_puts("usage: led on|off|blink\r\n");
    }
}

