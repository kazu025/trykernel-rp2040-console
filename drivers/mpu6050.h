#ifndef MPU6050_H
#define MPU6050_H

#include <trykernel.h>

#define MPU6050_I2C_ADDR  0x68U
#define MPU6050_WHO_AM_I   0x68U
#define MPU6500_WHO_AM_I   0x70U

typedef struct {
    INT accel_x;
    INT accel_y;
    INT accel_z;
    INT temperature;
    INT gyro_x;
    INT gyro_y;
    INT gyro_z;
} mpu6050_raw_data_t;

BOOL mpu6050_read_who_am_i(UB *device_id);
BOOL mpu6050_is_supported_device(UB device_id);
BOOL mpu6050_init(void);
BOOL mpu6050_read_raw(mpu6050_raw_data_t *raw_data);

#endif /* MPU6050_H */
