#include "typedef.h"
#include "sysdef.h"
#include "gpio.h"
#include "syslib.h"

extern void out_w(UW addr, UW data);
extern UW in_w(UW addr);

static unsigned int gpio_irq_pin;
static UW gpio_irq_group;
static UW gpio_irq_mask;
static GPIO_IRQ_NOTIFY_FUNC gpio_irq_notify;
static volatile UW gpio_irq_count;
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

BOOL gpio_irq_init_rising(unsigned int pin, GPIO_IRQ_NOTIFY_FUNC notify)
{
    UW pad;
    UW shift;
    UW intsts;

    if(!gpio_is_valid(pin) || (notify == NULL)){
        return FALSE;
    }

    gpio_reset_release();
    gpio_irq_pin = pin;
    gpio_irq_group = pin / 8U;
    shift = (pin % 8U) * 4U;
    gpio_irq_mask = 1UL << (shift + 3U);
    gpio_irq_notify = notify;
    gpio_irq_count = 0U;

    out_w(GPIO_CTRL(pin), GPIO_CTRL_FUNCSEL_SIO);
    out_w(GPIO_OE_CLR, 1UL << pin);
    pad = in_w(GPIO(pin));
    pad |= GPIO_IE | GPIO_PDE;
    pad &= ~(GPIO_OD | GPIO_PUE);
    out_w(GPIO(pin), pad);

    DI(intsts);
    out_w(IO_BANK0_INTR(gpio_irq_group), gpio_irq_mask);
    out_w(
        IO_BANK0_PROC0_INTE(gpio_irq_group),
        in_w(IO_BANK0_PROC0_INTE(gpio_irq_group)) | gpio_irq_mask
    );
    out_w(NVIC_ICPR, IO_IRQ_BANK0_MASK);
    out_w(NVIC_ISER, IO_IRQ_BANK0_MASK);
    EI(intsts);

    return TRUE;
}

UW gpio_irq_count_get(void)
{
    return gpio_irq_count;
}

void io_irq_bank0_handler(void)
{
    UW status;

    status = in_w(IO_BANK0_PROC0_INTS(gpio_irq_group));
    if((status & gpio_irq_mask) != 0U){
        out_w(IO_BANK0_INTR(gpio_irq_group), gpio_irq_mask);
        gpio_irq_count++;
        if(gpio_irq_notify != NULL){
            gpio_irq_notify();
        }
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
/*
 * GPIOにI2C機能を追加
 */
void gpio_init_i2c(unsigned int pin)
{
    UW pad;

    if(!gpio_is_valid(pin)) return;

    gpio_reset_release();

    /* GPIOをI2C機能へ接続 */
    out_w(GPIO_CTRL(pin), GPIO_CTRL_FUNCSEL_I2C);

    /*
     * 入力を有効化し、内部プルアップを有効化
     * 出力禁止とプルダウンは解除
     */
    pad = in_w(GPIO(pin));
    pad |= GPIO_IE | GPIO_PUE;
    pad &= ~(GPIO_OD | GPIO_PDE);
    out_w(GPIO(pin), pad);
}
