#ifndef W25QXX_H
#define W25QXX_H

#include <trykernel.h>

typedef struct {
    UB manufacturer_id;
    UB memory_type;
    UB capacity_id;
} w25qxx_jedec_id_t;

void w25qxx_init(void);
BOOL w25qxx_read_jedec_id(w25qxx_jedec_id_t *jedec_id);
const char *w25qxx_manufacturer_name(UB manufacturer_id);
UW w25qxx_capacity_bytes(UB capacity_id);

#endif /* W25QXX_H */
