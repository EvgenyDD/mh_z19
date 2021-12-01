#ifndef DHT22_H
#define DHT22_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_TICS 10000

#define DHT11_OK 0
#define DHT11_NO_CONN 1
#define DHT11_CS_ERROR 2

void dht22_init(void);
int dht11_poll(uint8_t *buf);

void dht11_poll2(void);

#endif // DHT22_H