/*
 * ADT7410 temperature sensor driver
 */
#include <trykernel.h>
#include "i2c.h"
#include "adt7410.h"

#define ADT7410_REG_TEMPERATURE  0x00U

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
    UINT raw_unsigned;
    INT raw_signed;

    if(temperature_milli_c == NULL){
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

    /*
     * デフォルトの13ビット形式では、
     * 下位3ビットはTCRIT、THIGH、TLOWの状態フラグ
     */
    raw_unsigned =
        (((UINT)data[0] << 8)
        | (UINT)data[1])
        & 0xFFF8U;

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