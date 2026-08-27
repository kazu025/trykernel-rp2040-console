/*
 * ADT7410 temperature sensor driver
 */
#include <trykernel.h>
#include "i2c.h"
#include "adt7410.h"

#define ADT7410_REG_TEMPERATURE  0x00U
#define ADT7410_REG_CONFIGURATION 0x03U
#define ADT7410_REG_ID            0x0BU
#define ADT7410_CONFIG_RESOLUTION  (1U << 7)

/*
 * 8ビットレジスタを1つ読み出す
 */
static BOOL adt7410_read_register(UB register_address, UB *value)
{
    if(value == NULL){
        return FALSE;
    }

    return i2c0_write_read(
        ADT7410_I2C_ADDR,
        &register_address,
        1U,
        value,
        1U
    );
}

/*
 * 8ビットレジスタへ1つ書き込む
 */
static BOOL adt7410_write_register(UB register_address, UB value)
{
    UB data[2];

    data[0] = register_address;
    data[1] = value;

    return i2c0_write(ADT7410_I2C_ADDR, data, 2U);
}

/*
 * 温度分解能を13ビットまたは16ビットへ切り替える
 */
BOOL adt7410_set_resolution(BOOL resolution_16bit)
{
    UB configuration;

    if(adt7410_read_register(
            ADT7410_REG_CONFIGURATION,
            &configuration) == FALSE){
        return FALSE;
    }

    if(resolution_16bit != FALSE){
        configuration |= ADT7410_CONFIG_RESOLUTION;
    }else{
        configuration &= (UB)~ADT7410_CONFIG_RESOLUTION;
    }

    if(adt7410_write_register(
            ADT7410_REG_CONFIGURATION,
            configuration) == FALSE){
        return FALSE;
    }

    /* 書き込んだ設定を読み戻して確認する */
    if(adt7410_read_register(
            ADT7410_REG_CONFIGURATION,
            &configuration) == FALSE){
        return FALSE;
    }

    return (((configuration & ADT7410_CONFIG_RESOLUTION) != 0U)
        == (resolution_16bit != FALSE));
}

/*
 * デバイスIDと設定レジスタを取得する
 */
BOOL adt7410_read_device_info(UB *device_id, UB *configuration)
{
    if((device_id == NULL) || (configuration == NULL)){
        return FALSE;
    }

    if(adt7410_read_register(ADT7410_REG_ID, device_id) == FALSE){
        return FALSE;
    }

    if(adt7410_read_register(
            ADT7410_REG_CONFIGURATION,
            configuration) == FALSE){
        return FALSE;
    }

    return TRUE;
}

/*
 * 温度をミリ℃単位で取得する
 *
 * 例:
 *   25125 = 25.125℃
 *   -5500 = -5.500℃
 */
BOOL adt7410_read_temperature(INT *temperature_milli_c)
{
    UB register_address;
    UB data[2];
    UB configuration;
    UINT raw_unsigned;
    INT raw_signed;

    if(temperature_milli_c == NULL){
        return FALSE;
    }

    if(adt7410_read_register(
            ADT7410_REG_CONFIGURATION,
            &configuration) == FALSE){
        return FALSE;
    }

    register_address = ADT7410_REG_TEMPERATURE;

    if(i2c0_write_read(
            ADT7410_I2C_ADDR,
            &register_address,
            1U,
            data,
            2U) == FALSE){
        return FALSE;
    }

    raw_unsigned =
        (((UINT)data[0] << 8)
        | (UINT)data[1]);

    /* 13ビット形式では下位3ビットは状態フラグ */
    if((configuration & ADT7410_CONFIG_RESOLUTION) == 0U){
        raw_unsigned &= 0xFFF8U;
    }

    /* 16ビット2の補数をINTへ変換 */
    if((raw_unsigned & 0x8000U) != 0U){
        raw_signed = (INT)raw_unsigned - 65536;
    }else{
        raw_signed = (INT)raw_unsigned;
    }

    if(raw_signed >= 0){
        *temperature_milli_c = (raw_signed * 1000 + 64) / 128;
    }else{
        *temperature_milli_c = -(((-raw_signed) * 1000 + 64) / 128);
    }
    return TRUE;
}
