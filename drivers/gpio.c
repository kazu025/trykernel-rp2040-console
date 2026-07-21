#include "typedef.h"
#include "sysdef.h"
#include "gpio.h"
#include "syslib.h"

extern void out_w(UW addr, UW data);
extern UW in_w(UW addr);
/*
 * GPIO pin munber check
 * pin : 0〜29
 */
static int gpio_is_valid(unsigned int pin){
    return (pin < 30);
}
/*
 * IO bank0/PADS bank0のリセット解除
 * GPIOを使用する前に必ず呼び出すこと。有効化する
 */
static void gpio_reset_release(void){
    UW mask;
    mask = RESETS_RESET_IO_BANK0 | RESETS_RESET_PADS_BANK0;
    /* reset bit をクリア: 該当ブロックのリセット解除 */
    out_w(RESETS_RESET, in_w(RESETS_RESET) & ~mask);
    /* RESET DONEになるまで待つ */
    while((in_w(RESETS_RESET_DONE) & mask) != mask);
}
/*
 * GPIOの出力として初期化
 * IO bank0/PADS bank0のリセット解除を行い、GPIOの出力として有効化する
 * GPIOx_CTRLのFUNCSELをSIOに設定
 */
void gpio_init_out(unsigned int pin){
    if(!gpio_is_valid(pin)) return;
    gpio_reset_release();
    /* GPIO pinをSIO機能に接続する */
    out_w(GPIO_CTRL(pin), GPIO_CTRL_FUNCSEL_SIO);
    /* GPIOの出力として有効化 */
    out_w(GPIO_OE_SET, 1U<<pin);
}
/*
 * GPIO出力をHighに設定する。
 */
void gpio_set(unsigned int pin){
    if(!gpio_is_valid(pin)) return;
    out_w(GPIO_OUT_SET, 1U<<pin);
}
/*
 * GPIO出力をLowに設定する。
 */
void gpio_clear(unsigned int pin){
    if(!gpio_is_valid(pin)) return;
    out_w(GPIO_OUT_CLR, 1U<<pin);
}
/*
 * GPIO出力を反転する。
 */
void gpio_toggle(unsigned int pin){
    if(!gpio_is_valid(pin)) return;
    out_w(GPIO_OUT_XOR, 1U<<pin);
}
/*
 * GPIO出力値を設定する
*/
void gpio_put(unsigned int pin, int value){
    if(!gpio_is_valid(pin)) return;
    if(value){
        out_w(GPIO_OUT_SET, 1U<<pin);
    }else{
        out_w(GPIO_OUT_CLR, 1U<<pin);
    }
}
/*
 * Pico onboard LED: GPIO25
 */
void led25_init(void){
    gpio_init_out(PICO_LED_PIN);
}
void led25_on(void){
    gpio_set(PICO_LED_PIN);
}
void led25_off(void){
    gpio_clear(PICO_LED_PIN);
}
void led25_toggle(void){
    gpio_toggle(PICO_LED_PIN);
}
