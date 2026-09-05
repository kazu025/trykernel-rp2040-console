#include <trykernel.h>
#include "gpio.h"
#include "spi.h"
#include "w25qxx.h"

#define W25QXX_CS_PIN             17U
#define W25QXX_CMD_JEDEC_ID       0x9FU
#define W25QXX_DUMMY_DATA         0xFFU

void w25qxx_init(void)
{
    gpio_init_out(W25QXX_CS_PIN);
    gpio_set(W25QXX_CS_PIN);
}

BOOL w25qxx_read_jedec_id(w25qxx_jedec_id_t *jedec_id)
{
    UB discard;
    BOOL result;

    if(jedec_id == NULL) return FALSE;

    gpio_clear(W25QXX_CS_PIN);
    result = spi0_transfer(W25QXX_CMD_JEDEC_ID, &discard)
        && spi0_transfer(W25QXX_DUMMY_DATA, &jedec_id->manufacturer_id)
        && spi0_transfer(W25QXX_DUMMY_DATA, &jedec_id->memory_type)
        && spi0_transfer(W25QXX_DUMMY_DATA, &jedec_id->capacity_id);
    gpio_set(W25QXX_CS_PIN);

    return result;
}

const char *w25qxx_manufacturer_name(UB manufacturer_id)
{
    switch(manufacturer_id){
    case 0xEFU:
        return "Winbond";
    case 0xC8U:
        return "GigaDevice";
    case 0xC2U:
        return "Macronix";
    case 0x20U:
        return "Micron";
    case 0x1CU:
        return "Eon";
    default:
        return "Unknown";
    }
}

UW w25qxx_capacity_bytes(UB capacity_id)
{
    if((capacity_id < 0x10U) || (capacity_id > 0x1FU)){
        return 0U;
    }

    return 1UL << capacity_id;
}
