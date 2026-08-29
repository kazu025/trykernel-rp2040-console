#ifndef RP2040_GPIO_H
#define RP2040_GPIO_H
/*
 * RP2040 GPIOドライバー
 * RP2040のレジスタを直接操作する最小GPIOドライバー
 */
/* --- 汎用GPIO API --- */
void gpio_init_out(unsigned int pin);
void gpio_set(unsigned int pin);
void gpio_clear(unsigned int pin);
void gpio_toggle(unsigned int pin);
void gpio_put(unsigned int pin, int value);

typedef void (*GPIO_IRQ_NOTIFY_FUNC)(void);
BOOL gpio_irq_init_rising(unsigned int pin, GPIO_IRQ_NOTIFY_FUNC notify);
UW gpio_irq_count_get(void);
void io_irq_bank0_handler(void);

/* --- Pico Onboard LED --- */
#define PICO_LED_PIN 25
void led25_init(void);
void led25_on(void);
void led25_off(void);
void led25_toggle(void);
/* --- I2C --- */
void gpio_init_i2c(unsigned int pin);
/* -------------------- */
#endif // RP2040_GPIO_H
