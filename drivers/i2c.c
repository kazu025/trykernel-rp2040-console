/*
 * I2C driver for TryKernel / RP2040 I2C0
 *
 * I2C0:
 *   SDA = GPIO4
 *   SCL = GPIO5
 *   Clock = 100 kHz
 */
#include <trykernel.h>
#include <knldef.h>
#include "gpio.h"
#include "i2c.h"

/*
 * clk_sys = 125 MHz、I2C = 100 kHz
 *
 * period = 125000000 / 100000 = 1250
 * High   = 1250 * 2 / 5 = 500
 * Low    = 1250 * 3 / 5 = 750
 */
#define I2C0_FS_SCL_HCNT    500U
#define I2C0_FS_SCL_LCNT    750U
#define I2C0_SDA_HOLD       38U
#define I2C0_FS_SPKLEN      46U

#define I2C0_TIMEOUT_LOOP   100000U

/*
 * I2C0の有効／無効を切り替える
 */
static BOOL i2c0_set_enabled(BOOL enable)
{
    UW expected;
    UW count;

    expected = (enable != FALSE) ? I2C_ENABLE_EN : 0U;

    out_w(
        I2C0_BASE + I2Cx_ENABLE,
        expected
    );

    for(count = 0; count < I2C0_TIMEOUT_LOOP; count++){
        if((in_w(I2C0_BASE + I2Cx_ENABLE_STATUS)
                & I2C_ENABLE_EN) == expected){
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * I2C0初期化
 */
void i2c0_init(void)
{
    UW reset_mask;

    reset_mask = RESETS_RESET_I2C0;

    /* I2C0をリセット */
    out_w(
        RESETS_RESET,
        in_w(RESETS_RESET) | reset_mask
    );

    /* I2C0のリセット解除 */
    out_w(
        RESETS_RESET,
        in_w(RESETS_RESET) & ~reset_mask
    );

    while((in_w(RESETS_RESET_DONE) & reset_mask) == 0U);

    /* GPIO4/5をI2C機能に設定 */
    gpio_init_i2c(I2C0_SDA_PIN);
    gpio_init_i2c(I2C0_SCL_PIN);

    (void)i2c0_set_enabled(FALSE);

    /*
     * マスター、Fast mode設定、RESTART有効、
     * スレーブ機能無効
     */
    out_w(
        I2C0_BASE + I2Cx_CON,
        I2C_CON_MASTER_MODE
        | I2C_CON_SPEED_FAST
        | I2C_CON_RESTART_EN
        | I2C_CON_SLAVE_DISABLE
        | I2C_CON_TX_EMPTY_CTRL
    );

    /* 100kHz用SCLタイミング */
    out_w(
        I2C0_BASE + I2Cx_FS_SCL_HCNT,
        I2C0_FS_SCL_HCNT
    );

    out_w(
        I2C0_BASE + I2Cx_FS_SCL_LCNT,
        I2C0_FS_SCL_LCNT
    );

    out_w(
        I2C0_BASE + I2Cx_SDA_HOLD,
        I2C0_SDA_HOLD
    );

    out_w(
        I2C0_BASE + I2Cx_FS_SPKLEN,
        I2C0_FS_SPKLEN
    );

    /* FIFOしきい値 */
    out_w(I2C0_BASE + I2Cx_RX_TL, 0U);
    out_w(I2C0_BASE + I2Cx_TX_TL, 0U);

    /* 今回はポーリング方式なので割込みを使用しない */
    out_w(I2C0_BASE + I2Cx_INTR_MASK, 0U);

    /* 残っている割込み状態をクリア */
    (void)in_w(I2C0_BASE + I2Cx_CLR_INTR);

    (void)i2c0_set_enabled(TRUE);
}

/*
 * 指定した7ビットアドレスから
 * 1バイトのダミー読み出しを行い、ACKを確認する
 */
BOOL i2c0_probe(UB addr)
{
    UW count;
    UW raw_status;
    BOOL acknowledged = FALSE;
    BOOL stopped = FALSE;

    /* I2C予約アドレスを除外 */
    if((addr < 0x08U) || (addr > 0x77U)){
        return FALSE;
    }

    if(i2c0_set_enabled(FALSE) == FALSE){
        return FALSE;
    }

    /* 前回の割込み状態をクリア */
    (void)in_w(I2C0_BASE + I2Cx_CLR_INTR);

    /* 通信相手のアドレスを設定 */
    out_w(I2C0_BASE + I2Cx_TAR, (UW)addr);

    if(i2c0_set_enabled(TRUE) == FALSE){
        return FALSE;
    }

    /* 1バイト読み出し後、STOP条件を生成 */
    out_w(
        I2C0_BASE + I2Cx_DATA_CMD,
        I2C_DATA_CMD_READ | I2C_DATA_CMD_STOP
    );

    for(count = 0; count < I2C0_TIMEOUT_LOOP; count++){
        raw_status = in_w(I2C0_BASE + I2Cx_RAW_INTR_STAT);

        /* アドレスNACKなどによる送信中断 */
        if((raw_status & I2C_RAW_TX_ABRT) != 0U){
            (void)in_w(I2C0_BASE + I2Cx_CLR_TX_ABRT);
            break;
        }

        /* 受信データがあればACKされたと判断 */
        if(in_w(I2C0_BASE + I2Cx_RXFLR) != 0U){
            (void)in_w(I2C0_BASE + I2Cx_DATA_CMD);
            acknowledged = TRUE;
        }

        /* STOP条件が完了 */
        if((raw_status & I2C_RAW_STOP_DET) != 0U){
            (void)in_w(I2C0_BASE + I2Cx_CLR_STOP_DET);
            stopped = TRUE;
            break;
        }
    }

    (void)i2c0_set_enabled(FALSE);

    return ((acknowledged != FALSE) && (stopped != FALSE));
}