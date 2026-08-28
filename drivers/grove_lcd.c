/*
 * Grove LCD RGB Backlight V5.0 driver
 *
 * LCD controller I2C address: 0x3E
 * RGB controller I2C address: 0x30
 *
 * Register sequences are based on Seeed Studio's official
 * Grove_LCD_RGB_Backlight library.
 */
#include <trykernel.h>
#include "i2c.h"
#include "grove_lcd.h"

#define GROVE_LCD_ADDRESS        0x3EU
#define GROVE_RGB_ADDRESS_V5     0x30U

#define GROVE_LCD_CONTROL_CMD    0x80U
#define GROVE_LCD_CONTROL_DATA   0x40U

#define GROVE_LCD_CMD_CLEAR      0x01U      // 画面クリア&カー位置を先頭に戻す
#define GROVE_LCD_CMD_ENTRY_MODE 0x06U      // カーソル移動方向設定 左→右
#define GROVE_LCD_CMD_DISPLAY_ON 0x0CU      // 表示ON カーソルOFF 点滅OFF
#define GROVE_LCD_CMD_FUNCTION   0x28U      // 機能設定

#define GROVE_RGB_REG_RESET      0x00U      // RESET レジスタ
#define GROVE_RGB_REG_LEDOUT     0x04U      // LEDOUT レジスタ
#define GROVE_RGB_REG_RED        0x06U      // RED レジスタ
#define GROVE_RGB_REG_GREEN      0x07U      // GREEN レジスタ
#define GROVE_RGB_REG_BLUE       0x08U      // BLUE レジスタ

// I2Cコマンド送信
static BOOL grove_lcd_write_pair(UB address, UB first, UB second)
{
    UB data[2];

    data[0] = first;
    data[1] = second;

    return i2c0_write(address, data, 2U);
}

// 文字表示を制御する
static BOOL grove_lcd_command(UB command)
{
    return grove_lcd_write_pair(
        GROVE_LCD_ADDRESS,
        GROVE_LCD_CONTROL_CMD,
        command
    );
}

// RGBバックライト側を制御する
static BOOL grove_rgb_write_register(UB reg, UB value)
{
    return grove_lcd_write_pair(
        GROVE_RGB_ADDRESS_V5,
        reg,
        value
    );
}
// 画面クリアとカーソルを先頭に移動
BOOL grove_lcd_clear(void)
{
    if(grove_lcd_command(GROVE_LCD_CMD_CLEAR) == FALSE){
        return FALSE;
    }

    tk_dly_tsk(10);
    return TRUE;
}
// カーソル位置設定
BOOL grove_lcd_set_cursor(UB column, UB row)
{
    UB address;

    if((column >= GROVE_LCD_COLUMNS) || (row >= GROVE_LCD_ROWS)){
        return FALSE;
    }

    address = (row == 0U)
        ? (UB)(0x80U + column)
        : (UB)(0xC0U + column);

    return grove_lcd_command(address);
}
// テキスト設定
BOOL grove_lcd_write_text(const char *text)
{
    if(text == NULL){
        return FALSE;
    }

    while(*text != '\0'){
        if(grove_lcd_write_pair(
                GROVE_LCD_ADDRESS,
                GROVE_LCD_CONTROL_DATA,
                (UB)*text) == FALSE){
            return FALSE;
        }
        text++;
    }

    return TRUE;
}
// RGBバックライト色設定
BOOL grove_lcd_set_rgb(UB red, UB green, UB blue)
{
    /* 単独で呼び出しても動作するようRGBドライバを初期化する */
    if(grove_rgb_write_register(GROVE_RGB_REG_RESET, 0x07U) == FALSE){
        return FALSE;
    }
    tk_dly_tsk(10);

    if(grove_rgb_write_register(GROVE_RGB_REG_LEDOUT, 0x15U) == FALSE){
        return FALSE;
    }

    if(grove_rgb_write_register(GROVE_RGB_REG_RED, red) == FALSE){
        return FALSE;
    }
    if(grove_rgb_write_register(GROVE_RGB_REG_GREEN, green) == FALSE){
        return FALSE;
    }
    if(grove_rgb_write_register(GROVE_RGB_REG_BLUE, blue) == FALSE){
        return FALSE;
    }

    return TRUE;
}
// 初期化
BOOL grove_lcd_init(void)
{
    /* LCD電源投入後の待ち時間 */
    tk_dly_tsk(50);

    if(grove_lcd_command(GROVE_LCD_CMD_FUNCTION) == FALSE){
        return FALSE;
    }
    tk_dly_tsk(10);

    if(grove_lcd_command(GROVE_LCD_CMD_FUNCTION) == FALSE){
        return FALSE;
    }
    tk_dly_tsk(10);

    if(grove_lcd_command(GROVE_LCD_CMD_FUNCTION) == FALSE){
        return FALSE;
    }
    if(grove_lcd_command(GROVE_LCD_CMD_FUNCTION) == FALSE){
        return FALSE;
    }
    if(grove_lcd_command(GROVE_LCD_CMD_DISPLAY_ON) == FALSE){
        return FALSE;
    }
    if(grove_lcd_clear() == FALSE){
        return FALSE;
    }
    if(grove_lcd_command(GROVE_LCD_CMD_ENTRY_MODE) == FALSE){
        return FALSE;
    }

    return grove_lcd_set_rgb(64U, 128U, 255U);
}
