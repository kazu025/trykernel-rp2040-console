#include <trykernel.h>
#include "task_led.h"
#include "command.h"
#include "uart_tx.h"
#include "mini_printf.h"
#include "uart.h"
#include "gpio.h"
#include "i2c.h"
#include "adt7410.h"
#include "mpu6050.h"
#include "grove_lcd.h"

/* --- コマンドバッファ最大数 --- */
#define CMD_MAX_ARGS    16
#define CMD_OUTPUT_BUF_SIZE 128U
static void cmd_help(int argc, char *argv[]);
static void cmd_status(int argc, char *argv[]);
static void cmd_echo(int argc, char *argv[]);
static void cmd_led(int argc, char *argv[]);
static void cmd_print(int argc, char *argv[]);
static int split_args(char *line, char *argv[], int max_args);
static void cmd_i2cscan(int argc, char *argv[]);
static void cmd_temperature(int argc, char *argv[]);
static void cmd_adtinfo(int argc, char *argv[]);
static void cmd_adtconfig(int argc, char *argv[]);
static void cmd_adtraw(int argc, char *argv[]);
static void cmd_mpuid(int argc, char *argv[]);
static void cmd_mpuraw(int argc, char *argv[]);
static void cmd_mpu(int argc, char *argv[]);
static void cmd_lcdtest(int argc, char *argv[]);
static void cmd_lcdtemp(int argc, char *argv[]);
static void cmd_lcdcolor(int argc, char *argv[]);
static void format_temperature_line(INT temperature_milli_c, char *line);
static BOOL parse_u8(const char *text, UB *value);
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
    {"print",   cmd_print,  "print test"},
    {"i2cscan", cmd_i2cscan, "scan I2C devices"},
    {"temperature", cmd_temperature, "read temperature from ADT7410 sensor"},
    {"adtinfo", cmd_adtinfo, "show ADT7410 ID and configuration"},
    {"adtconfig", cmd_adtconfig, "set ADT7410 resolution: 13|16"},
    {"adtraw", cmd_adtraw, "show ADT7410 raw temperature data"},
    {"mpuid", cmd_mpuid, "show MPU-6050 WHO_AM_I"},
    {"mpuraw", cmd_mpuraw, "show MPU-6050 raw sensor data"},
    {"mpu", cmd_mpu, "show acceleration, gyro and temperature"},
    {"lcdtest", cmd_lcdtest, "test Grove RGB LCD V5.0"},
    {"lcdtemp", cmd_lcdtemp, "show ADT7410 temperature on LCD"},
    {"lcdcolor", cmd_lcdcolor, "set LCD backlight: R G B"}
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

static BOOL parse_u8(const char *text, UB *value)
{
    UINT number = 0U;

    if((text == NULL) || (value == NULL) || (*text == '\0')){
        return FALSE;
    }

    while(*text != '\0'){
        UINT digit;

        if((*text < '0') || (*text > '9')){
            return FALSE;
        }

        digit = (UINT)(*text - '0');
        if((number > 25U) || ((number == 25U) && (digit > 5U))){
            return FALSE;
        }

        number = (number * 10U) + digit;
        text++;
    }

    *value = (UB)number;
    return TRUE;
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
    uart_tx_printf("UART RX IRQ count: %u\r\n", (UINT)uart_rx_irq_count_get());
    uart_tx_printf("UART RX timeout IRQ count: %u\r\n", (UINT)uart_rt_irq_count_get());
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
static void cmd_i2cscan(int argc, char* argv[]){
    UW found = 0U;
    (void)argc;
    (void)argv;
    uart_tx_send("Scanning I2C bus...\r\n");
    for(UB addr = 0x08; addr <= 0x77; addr++){
        if(i2c0_probe(addr)){
            uart_tx_printf("Found device at 0x%x\r\n", addr);
            found++;
        }
    }
    uart_tx_printf("I2C scan complete. Found %u device(s).\r\n", (UINT)found);
}
static void cmd_temperature(int argc, char *argv[])
{
    INT temperature_milli_c;
    UINT magnitude;
    UINT integer_part;
    UINT fractional_part;
    const char *sign;

    (void)argc;
    (void)argv;

    if(adt7410_read_temperature(
            &temperature_milli_c) == FALSE){
        uart_tx_send("ADT7410 read error\r\n");
        return;
    }

    if(temperature_milli_c < 0){
        sign = "-";
        magnitude = (UINT)(-temperature_milli_c);
    }else{
        sign = "";
        magnitude = (UINT)temperature_milli_c;
    }

    integer_part = magnitude / 1000U;
    fractional_part = magnitude % 1000U;

    if(fractional_part < 10U){
        uart_tx_printf(
            "ADT7410 temperature: %s%u.00%u C\r\n",
            sign,
            integer_part,
            fractional_part
        );
    }else if(fractional_part < 100U){
        uart_tx_printf(
            "ADT7410 temperature: %s%u.0%u C\r\n",
            sign,
            integer_part,
            fractional_part
        );
    }else{
        uart_tx_printf(
            "ADT7410 temperature: %s%u.%u C\r\n",
            sign,
            integer_part,
            fractional_part
        );
    }
}

static void cmd_adtinfo(int argc, char *argv[])
{
    UB device_id;
    UB configuration;
    UB operation_mode;
    const char *operation_mode_name;

    (void)argc;
    (void)argv;

    if(adt7410_read_device_info(
            &device_id,
            &configuration) == FALSE){
        uart_tx_send("ADT7410 diagnostic read error\r\n");
        return;
    }

    operation_mode = (configuration >> 5) & 0x03U;
    if(operation_mode == 0U){
        operation_mode_name = "continuous";
    }else if(operation_mode == 1U){
        operation_mode_name = "one shot";
    }else if(operation_mode == 2U){
        operation_mode_name = "1 SPS";
    }else{
        operation_mode_name = "shutdown";
    }

    uart_tx_printf("ADT7410 ID: 0x%x\r\n", (UINT)device_id);
    uart_tx_printf(
        "  manufacturer ID: 0x%x\r\n",
        (UINT)(device_id >> 3)
    );
    uart_tx_printf(
        "  revision ID: 0x%x\r\n",
        (UINT)(device_id & 0x07U)
    );
    uart_tx_printf(
        "ADT7410 configuration: 0x%x\r\n",
        (UINT)configuration
    );
    uart_tx_printf(
        "  resolution: %s\r\n",
        ((configuration & 0x80U) != 0U) ? "16 bit" : "13 bit"
    );
    uart_tx_printf(
        "  operation mode: %s\r\n",
        operation_mode_name
    );
}

static void cmd_adtconfig(int argc, char *argv[])
{
    BOOL resolution_16bit;

    if(argc != 2){
        uart_tx_send("usage: adtconfig 13|16\r\n");
        return;
    }

    if(str_eq(argv[1], "13") == TRUE){
        resolution_16bit = FALSE;
    }else if(str_eq(argv[1], "16") == TRUE){
        resolution_16bit = TRUE;
    }else{
        uart_tx_send("usage: adtconfig 13|16\r\n");
        return;
    }

    if(adt7410_set_resolution(resolution_16bit) == FALSE){
        uart_tx_send("ADT7410 configuration error\r\n");
        return;
    }

    uart_tx_printf(
        "ADT7410 resolution: %s\r\n",
        (resolution_16bit != FALSE) ? "16 bit" : "13 bit"
    );
}

static void cmd_adtraw(int argc, char *argv[])
{
    UINT raw_temperature;
    BOOL resolution_16bit;

    (void)argc;
    (void)argv;

    if(adt7410_read_raw_temperature(
            &raw_temperature,
            &resolution_16bit) == FALSE){
        uart_tx_send("ADT7410 raw read error\r\n");
        return;
    }

    uart_tx_printf(
        "ADT7410 raw temperature: 0x%x\r\n",
        raw_temperature
    );
    uart_tx_printf(
        "  resolution: %s\r\n",
        (resolution_16bit != FALSE) ? "16 bit" : "13 bit"
    );

    if(resolution_16bit == FALSE){
        uart_tx_printf(
            "  flags: TLOW=%u THIGH=%u TCRIT=%u\r\n",
            raw_temperature & 0x01U,
            (raw_temperature >> 1) & 0x01U,
            (raw_temperature >> 2) & 0x01U
        );
    }
}

static void cmd_mpuid(int argc, char *argv[])
{
    UB device_id;

    (void)argc;
    (void)argv;

    if(mpu6050_read_who_am_i(&device_id) == FALSE){
        uart_tx_send("MPU-6050 WHO_AM_I read error\r\n");
        return;
    }

    uart_tx_printf("MPU-6050 WHO_AM_I: 0x%x\r\n", (UINT)device_id);

    if(device_id == MPU6050_WHO_AM_I){
        uart_tx_send("  device: MPU-6050\r\n");
    }else if(device_id == MPU6500_WHO_AM_I){
        uart_tx_send("  device: MPU-6500 compatible\r\n");
    }else{
        uart_tx_send("  unexpected device ID\r\n");
    }
}

static void cmd_mpuraw(int argc, char *argv[])
{
    mpu6050_raw_data_t raw_data;

    (void)argc;
    (void)argv;

    if(mpu6050_init() == FALSE){
        uart_tx_send("MPU-6050 initialization error\r\n");
        return;
    }

    if(mpu6050_read_raw(&raw_data) == FALSE){
        uart_tx_send("MPU-6050 raw data read error\r\n");
        return;
    }

    uart_tx_printf(
        "MPU-6050 accel raw: X=%d Y=%d Z=%d\r\n",
        raw_data.accel_x,
        raw_data.accel_y,
        raw_data.accel_z
    );
    uart_tx_printf(
        "MPU-6050 gyro raw:  X=%d Y=%d Z=%d\r\n",
        raw_data.gyro_x,
        raw_data.gyro_y,
        raw_data.gyro_z
    );
    uart_tx_printf(
        "MPU-6050 temperature raw: %d\r\n",
        raw_data.temperature
    );
}

static void format_milli_value(
    INT value,
    const char *unit,
    char *text,
    UW text_size
)
{
    UINT magnitude;
    UINT integer_part;
    UINT fractional_part;
    const char *sign;

    if(value < 0){
        sign = "-";
        magnitude = (UINT)(-value);
    }else{
        sign = "";
        magnitude = (UINT)value;
    }

    integer_part = magnitude / 1000U;
    fractional_part = magnitude % 1000U;

    /* mini_printfは桁指定に対応していないため手動でゼロを補う */
    if(fractional_part < 10U){
        mini_snprintf(
            text,
            text_size,
            "%s%u.00%u %s",
            sign,
            integer_part,
            fractional_part,
            unit
        );
    }else if(fractional_part < 100U){
        mini_snprintf(
            text,
            text_size,
            "%s%u.0%u %s",
            sign,
            integer_part,
            fractional_part,
            unit
        );
    }else{
        mini_snprintf(
            text,
            text_size,
            "%s%u.%u %s",
            sign,
            integer_part,
            fractional_part,
            unit
        );
    }
}

static void cmd_mpu(int argc, char *argv[])
{
    mpu6050_raw_data_t raw_data;
    UB device_id;
    INT accel_x_milli_g;
    INT accel_y_milli_g;
    INT accel_z_milli_g;
    INT gyro_x_milli_dps;
    INT gyro_y_milli_dps;
    INT gyro_z_milli_dps;
    INT temperature_milli_c;
    char accel_x_text[24];
    char accel_y_text[24];
    char accel_z_text[24];
    char gyro_x_text[24];
    char gyro_y_text[24];
    char gyro_z_text[24];
    char temperature_text[24];

    (void)argc;
    (void)argv;

    if(mpu6050_read_who_am_i(&device_id) == FALSE
            || mpu6050_is_supported_device(device_id) == FALSE){
        uart_tx_send("MPU sensor identification error\r\n");
        return;
    }

    if(mpu6050_init() == FALSE
            || mpu6050_read_raw(&raw_data) == FALSE){
        uart_tx_send("MPU sensor read error\r\n");
        return;
    }

    /* 初期レンジ: 加速度±2g、ジャイロ±250dps */
    accel_x_milli_g = (raw_data.accel_x * 1000) / 16384;
    accel_y_milli_g = (raw_data.accel_y * 1000) / 16384;
    accel_z_milli_g = (raw_data.accel_z * 1000) / 16384;
    gyro_x_milli_dps = (raw_data.gyro_x * 1000) / 131;
    gyro_y_milli_dps = (raw_data.gyro_y * 1000) / 131;
    gyro_z_milli_dps = (raw_data.gyro_z * 1000) / 131;

    if(device_id == MPU6500_WHO_AM_I){
        temperature_milli_c =
            (INT)(((D)raw_data.temperature * 100000) / 33387) + 21000;
    }else{
        temperature_milli_c =
            (raw_data.temperature * 50) / 17 + 36530;
    }

    format_milli_value(
        accel_x_milli_g, "g", accel_x_text, sizeof(accel_x_text));
    format_milli_value(
        accel_y_milli_g, "g", accel_y_text, sizeof(accel_y_text));
    format_milli_value(
        accel_z_milli_g, "g", accel_z_text, sizeof(accel_z_text));
    format_milli_value(
        gyro_x_milli_dps, "dps", gyro_x_text, sizeof(gyro_x_text));
    format_milli_value(
        gyro_y_milli_dps, "dps", gyro_y_text, sizeof(gyro_y_text));
    format_milli_value(
        gyro_z_milli_dps, "dps", gyro_z_text, sizeof(gyro_z_text));
    format_milli_value(
        temperature_milli_c,
        "C",
        temperature_text,
        sizeof(temperature_text)
    );

    uart_tx_printf(
        "Acceleration: X=%s Y=%s Z=%s\r\n",
        accel_x_text,
        accel_y_text,
        accel_z_text
    );
    uart_tx_printf(
        "Gyroscope:    X=%s Y=%s Z=%s\r\n",
        gyro_x_text,
        gyro_y_text,
        gyro_z_text
    );
    uart_tx_printf("Temperature:  %s\r\n", temperature_text);
}

static void cmd_lcdtest(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if(grove_lcd_init() == FALSE){
        uart_tx_send("Grove LCD initialization error\r\n");
        return;
    }

    if(grove_lcd_set_cursor(0U, 0U) == FALSE
            || grove_lcd_write_text("Try Kernel") == FALSE
            || grove_lcd_set_cursor(0U, 1U) == FALSE
            || grove_lcd_write_text("LCD test") == FALSE){
        uart_tx_send("Grove LCD write error\r\n");
        return;
    }

    uart_tx_send("Grove LCD test complete\r\n");
}

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

static void cmd_lcdtemp(int argc, char *argv[])
{
    INT temperature_milli_c;
    char temperature_line[GROVE_LCD_COLUMNS + 1U];

    (void)argc;
    (void)argv;

    if(adt7410_read_temperature(&temperature_milli_c) == FALSE){
        uart_tx_send("ADT7410 read error\r\n");
        return;
    }

    format_temperature_line(temperature_milli_c, temperature_line);

    if(grove_lcd_init() == FALSE){
        uart_tx_send("Grove LCD initialization error\r\n");
        return;
    }

    if(grove_lcd_set_cursor(0U, 0U) == FALSE
            || grove_lcd_write_text("ADT7410 Temp    ") == FALSE
            || grove_lcd_set_cursor(0U, 1U) == FALSE
            || grove_lcd_write_text(temperature_line) == FALSE){
        uart_tx_send("Grove LCD write error\r\n");
        return;
    }

    uart_tx_send("ADT7410 temperature displayed on LCD\r\n");
}

static void cmd_lcdcolor(int argc, char *argv[])
{
    UB red;
    UB green;
    UB blue;

    if((argc != 4)
            || (parse_u8(argv[1], &red) == FALSE)
            || (parse_u8(argv[2], &green) == FALSE)
            || (parse_u8(argv[3], &blue) == FALSE)){
        uart_tx_send("usage: lcdcolor R G B (0-255)\r\n");
        return;
    }

    if(grove_lcd_set_rgb(red, green, blue) == FALSE){
        uart_tx_send("Grove LCD backlight error\r\n");
        return;
    }

    uart_tx_printf(
        "Grove LCD color: %u %u %u\r\n",
        (UINT)red,
        (UINT)green,
        (UINT)blue
    );
}
