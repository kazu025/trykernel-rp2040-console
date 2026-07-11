#ifndef RP2040_GPIO_H
#define RP2040_GPIO_H
/*
 * RP2040 GPIOドライバー
 * RP2040のレジスタを直セス操作する最小GPIOドライバー
 */
/* --- 汎用GPIO API --- */
void gpio_init_out(unsigned int pin);
void gpio_set(unsigned int pin);
void gpio_clear(unsigned int pin);
void gpio_toggle(unsigned int pin);
void gpio_put(unsigned int pin, int value);

/* --- Pico Oboard LED --- */
#define PICO_LED_PIN 25
void led25_init(void);
void led25_on(void);
void led25_off(void);
void led25_toggle(void);
/* -------------------- */
#endif // RP2040_GPIO_H