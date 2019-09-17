#pragma once

#include <types.h>

#define HUM_HIGH(x)  (x->byte[0])
#define HUM_LOW(x)   (x->byte[1])
#define TEMP_HIGH(x) (x->byte[2])
#define TEMP_LOW(x)  (x->byte[3])
#define PARITY(x)    (x->byte[4])
struct dht21 {
	u8 byte[5];
};

/**
 * Initializes PIN
 */
void dht21_init(gpio_num_t pin);

/**
 * Returns true if checksum is correct
 */
bool dht21_checksum_ok(struct dht21 *data);

/**
 * Polls the DHT21 device and parses the received signal into the data struct
 */
void dht21_poll_data(gpio_num_t pin, struct dht21 *data);

i16 dht21_temperature(struct dht21 *data);
u16 dht21_humidity(struct dht21 *data);

