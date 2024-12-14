#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_TICS 10000

#define DHT11_OK 0
#define DHT11_NO_CONN 1
#define DHT11_CS_ERROR 2

void dht11_init(void);
int dht11_poll(uint8_t *buf);

#endif // DHT11_H