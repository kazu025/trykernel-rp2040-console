#ifndef RP2040_I2C_H
#define RP2040_I2C_H

#include <trykernel.h>

#define I2C0_SDA_PIN  4U
#define I2C0_SCL_PIN  5U

void i2c0_init(void);
BOOL i2c0_probe(UB addr);
BOOL i2c0_write(
    UB addr,
    const UB *data,
    UINT size
);

BOOL i2c0_write_read(
    UB addr,
    const UB *write_data,
    UINT write_size,
    UB *read_data,
    UINT read_size
);

#endif /* RP2040_I2C_H */
