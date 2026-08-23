#ifndef ADT7410_H
#define ADT7410_H

#include <trykernel.h>

#define ADT7410_I2C_ADDR  0x48U

BOOL adt7410_read_temperature(INT *temperature_milli_c);

#endif /* ADT7410_H */