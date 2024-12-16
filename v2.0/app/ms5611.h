#ifndef MS5611_H__
#define MS5611_H__

#include <stdint.h>

typedef struct
{
	uint32_t d1;
	uint32_t d2;
	int32_t dt;
	int64_t off;
	int64_t sens;
	int32_t temp_0_01C;
	int32_t p_pa;
	int32_t p_0_01mmhg;
} ms5611_meas_t;

void ms5611_init(void);
void ms5611_poll(uint32_t diff_ms);

extern ms5611_meas_t ms5611_meas;

#endif // MS5611_H__