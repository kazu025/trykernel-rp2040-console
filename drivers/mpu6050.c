/*
 * MPU-6050 accelerometer and gyroscope driver
 */
#include <trykernel.h>
#include "i2c.h"
#include "mpu6050.h"

#define MPU6050_REG_ACCEL_XOUT_H  0x3BU
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_WHO_AM_I      0x75U

/*
 * 8ビットレジスタへ1つ書き込む
 */
static BOOL mpu6050_write_register(UB register_address, UB value)
{
    UB data[2];

    data[0] = register_address;
    data[1] = value;

    return i2c0_write(MPU6050_I2C_ADDR, data, 2U);
}

/*
 * ビッグエンディアンの16ビットデータを符号付き整数へ変換する
 */
static INT mpu6050_decode_s16(UB high, UB low)
{
    UINT value;

    value = ((UINT)high << 8) | (UINT)low;
    if((value & 0x8000U) != 0U){
        return (INT)value - 65536;
    }

    return (INT)value;
}

/*
 * WHO_AM_Iレジスタを読み出す
 */
BOOL mpu6050_read_who_am_i(UB *device_id)
{
    UB register_address;

    if(device_id == NULL){
        return FALSE;
    }

    register_address = MPU6050_REG_WHO_AM_I;

    return i2c0_write_read(
        MPU6050_I2C_ADDR,
        &register_address,
        1U,
        device_id,
        1U
    );
}

/*
 * このドライバで扱えるデバイスIDか確認する
 */
BOOL mpu6050_is_supported_device(UB device_id)
{
    return (device_id == MPU6050_WHO_AM_I)
        || (device_id == MPU6500_WHO_AM_I);
}

/*
 * スリープを解除する
 *
 * 初期設定では加速度レンジは±2g、ジャイロレンジは±250dps。
 */
BOOL mpu6050_init(void)
{
    return mpu6050_write_register(MPU6050_REG_PWR_MGMT_1, 0x00U);
}

/*
 * 加速度、温度、ジャイロの生データを連続読み出しする
 */
BOOL mpu6050_read_raw(mpu6050_raw_data_t *raw_data)
{
    UB register_address;
    UB data[14];

    if(raw_data == NULL){
        return FALSE;
    }

    register_address = MPU6050_REG_ACCEL_XOUT_H;
    if(i2c0_write_read(
            MPU6050_I2C_ADDR,
            &register_address,
            1U,
            data,
            14U) == FALSE){
        return FALSE;
    }

    raw_data->accel_x = mpu6050_decode_s16(data[0], data[1]);
    raw_data->accel_y = mpu6050_decode_s16(data[2], data[3]);
    raw_data->accel_z = mpu6050_decode_s16(data[4], data[5]);
    raw_data->temperature = mpu6050_decode_s16(data[6], data[7]);
    raw_data->gyro_x = mpu6050_decode_s16(data[8], data[9]);
    raw_data->gyro_y = mpu6050_decode_s16(data[10], data[11]);
    raw_data->gyro_z = mpu6050_decode_s16(data[12], data[13]);

    return TRUE;
}
