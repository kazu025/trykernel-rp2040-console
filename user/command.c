#include <trykernel.h>
#include "task_led.h"
#include "command.h"
#include "uart_tx.h"
#include "uart.h"
/* --- コマンドバッファ最大数 --- */
#define CMD_MAX_ARGS    16
#define CMD_OUTPUT_BUF_SIZE 128U
static void cmd_help(int argc, char *argv[]);
static void cmd_status(int argc, char *argv[]);
static void cmd_echo(int argc, char *argv[]);
static void cmd_led(int argc, char *argv[]);
static void cmd_print(int argc, char *argv[]);
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
    {"h",    cmd_help,   "show command list"},
    {"status",  cmd_status, "show system status"},
    {"echo",    cmd_echo,  "echo arguments"},
    {"led",     cmd_led,    "led on/off/blink"},
    {"print",   cmd_print,  "print test"}
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
    uart_tx_printf("unknown command: %s\r\n", argv[0]);
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
    uart_tx_send("commands:\r\n");
    for(int i=0; i<command_count; i++){
        uart_tx_printf("   %s -  %s\r\n", command_table[i].name, command_table[i].help);
    }
}
/*
 * コマンド:status　(ステータス表示)
 */
static void cmd_status(int argc, char *argv[]){
   UW overflow_count;
   (void)argc; (void)argv;
   overflow_count = uart_tx_get_overflow_count();
    uart_tx_send("TryKernel status: running\r\n");
    uart_tx_printf("LED mode: %s\r\n", led_task_mode_name());
    uart_tx_printf("UART TX queue: %s\r\n",
        overflow_count != 0U ? "overflow detected" : "OK") ;
    uart_tx_printf("UART TX overflow count: %u\r\n", (UINT)overflow_count);
    uart_tx_printf("UART RX overflow count: %u\r\n", (UINT)uart_rx_overflow_count());
    uart_tx_printf("UART RX HW overrun count: %u\r\n", (UINT)uart_rx_hw_overrun_count());
}
/*
 * コマンド:Echo (Echo 表示)
 */
static void cmd_echo(int argc, char *argv[]){
    char text[CMD_OUTPUT_BUF_SIZE];
    UW pos = 0U;
    /*
     * 引数を空白で連結 最後のCR/LF/NULLに3バイト残す
     */
    for(int i=1; i<argc; i++){
        const char *src = argv[i];
        while((*src != '\0') && (pos < (CMD_OUTPUT_BUF_SIZE - 3U))){
            text[pos++] = *src++;
        }
        if((i != (argc - 1)) && (pos < (CMD_OUTPUT_BUF_SIZE - 3U))){
            text[pos] = ' ';
            pos++;
        }
    }
    text[pos++] = '\r';
    text[pos++] = '\n';
    text[pos] = '\0';

    uart_tx_send(text);
}
/*
 * コマンド:LED (LED表示)
 */
static void cmd_led(int argc, char *argv[]){
    if (argc < 2){
        uart_tx_send("usage: led on|off|blink\r\n");
        return ;
    }else if (str_eq(argv[1], "on") == TRUE){
        led_task_set_on();
        uart_tx_send("led on\r\n");
    }else if (str_eq(argv[1], "off") == TRUE){
        led_task_set_off();
        uart_tx_send("led off\r\n");
    }else if (str_eq(argv[1], "blink") == TRUE){
        led_task_blink();
        uart_tx_send("led blink\r\n");
    }else{
        uart_tx_send("usage: led on|off|blink\r\n");
    }
}
/*
 * mini_printf()テスト
 */
static void cmd_print(int argc, char* argv[]){
    (void)argc;
    (void)argv;
    uart_tx_printf(
        "test: %s %c %d %u %x %%\r\n",
        "abc", 'Z', -123, 456U, 0x1a2b3cU
    );
}
