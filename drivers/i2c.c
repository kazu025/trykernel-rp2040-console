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
 * 指定した割込み状態になるまで待つ
 */
static BOOL i2c0_wait_raw_status(UW mask)
{
    UW count;
    UW raw_status;

    for(count = 0; count < I2C0_TIMEOUT_LOOP; count++){
        raw_status = in_w(
            I2C0_BASE + I2Cx_RAW_INTR_STAT
        );

        if((raw_status & I2C_RAW_TX_ABRT) != 0U){
            (void)in_w(
                I2C0_BASE + I2Cx_CLR_TX_ABRT
            );
            return FALSE;
        }

        if((raw_status & mask) != 0U){
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * RX FIFOから1バイト受信するまで待つ
 */
static BOOL i2c0_wait_read_byte(UB *data)
{
    UW count;
    UW raw_status;

    if(data == NULL){
        return FALSE;
    }

    for(count = 0; count < I2C0_TIMEOUT_LOOP; count++){
        raw_status = in_w(
            I2C0_BASE + I2Cx_RAW_INTR_STAT
        );

        if((raw_status & I2C_RAW_TX_ABRT) != 0U){
            (void)in_w(
                I2C0_BASE + I2Cx_CLR_TX_ABRT
            );
            return FALSE;
        }

        if(in_w(I2C0_BASE + I2Cx_RXFLR) != 0U){
            *data = (UB)(
                in_w(I2C0_BASE + I2Cx_DATA_CMD) & 0xFFU
            );
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * レジスタ番号などを書き込んだ後、
 * Repeated STARTでデータを読み出す
 */
BOOL i2c0_write_read(
    UB addr,
    const UB *write_data,
    UINT write_size,
    UB *read_data,
    UINT read_size
)
{
    UINT i;
    UW command;
    BOOL result = FALSE;

    if((addr < 0x08U) || (addr > 0x77U)){
        return FALSE;
    }

    if((write_data == NULL) || (write_size == 0U)){
        return FALSE;
    }

    if((read_data == NULL) || (read_size == 0U)){
        return FALSE;
    }

    if(i2c0_set_enabled(FALSE) == FALSE){
        return FALSE;
    }

    /* 前回の割込み状態をクリア */
    (void)in_w(I2C0_BASE + I2Cx_CLR_INTR);

    /* 通信相手を設定 */
    out_w(I2C0_BASE + I2Cx_TAR, (UW)addr);

    if(i2c0_set_enabled(TRUE) == FALSE){
        return FALSE;
    }

    /*
     * レジスタ番号などを書き込む。
     * 読み出しへ続くためSTOPは生成しない。
     */
    for(i = 0U; i < write_size; i++){
        out_w(
            I2C0_BASE + I2Cx_DATA_CMD,
            (UW)write_data[i]
        );

        if(i2c0_wait_raw_status(
                I2C_RAW_TX_EMPTY) == FALSE){
            goto cleanup;
        }
    }

    /*
     * 最初の読み出しでRepeated START、
     * 最後の読み出しでSTOPを生成する。
     */
    for(i = 0U; i < read_size; i++){
        command = I2C_DATA_CMD_READ;

        if(i == 0U){
            command |= I2C_DATA_CMD_RESTART;
        }

        if(i == (read_size - 1U)){
            command |= I2C_DATA_CMD_STOP;
        }

        out_w(
            I2C0_BASE + I2Cx_DATA_CMD,
            command
        );

        if(i2c0_wait_read_byte(
                &read_data[i]) == FALSE){
            goto cleanup;
        }
    }

    if(i2c0_wait_raw_status(
            I2C_RAW_STOP_DET) == FALSE){
        goto cleanup;
    }

    (void)in_w(
        I2C0_BASE + I2Cx_CLR_STOP_DET
    );

    result = TRUE;

cleanup:
    /* 残っている割込み状態をクリア */
    (void)in_w(I2C0_BASE + I2Cx_CLR_INTR);
    (void)i2c0_set_enabled(FALSE);

    return result;
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