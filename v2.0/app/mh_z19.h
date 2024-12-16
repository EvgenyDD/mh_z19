#ifndef MH_Z19_H__
#define MH_Z19_H__

#include <stdbool.h>
#include <stdint.h>

#define MH_Z19_INVALID_VALUE 0xFFFF

void mh_z19_init(void);

void mh_z19_poll(uint32_t diff_ms);
uint16_t mx_z19_get_co2(void);
uint16_t mx_z19_get_temp(void);

#endif // MH_Z19_H__