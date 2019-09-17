/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
   */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

typedef uint8_t  u8;  typedef int8_t  i8;
typedef uint16_t u16; typedef int16_t i16;
typedef uint32_t u32; typedef int32_t i32;
typedef uint64_t u64; typedef int64_t i64;

//#define DBG_SIGNAL
// #define DBG_CHKSUM

#define HIGH 1
#define LOW 0

#define LOG(fmt, ...) printf("LOG: " fmt "\n", ##__VA_ARGS__)

#define CHECK(code) if (code != ESP_OK) { LOG("ESP_ERR: %s", #code); }

/* 40 MHz crystal */
#define CLOCK_RATE_MHZ (240)
#define CLOCK_RATE_HZ (CLOCK_RATE_MHZ * 1000000)

#define BLINK_GPIO GPIO_NUM_2
#define DHT21_SENSOR_GPIO GPIO_NUM_25

#define vTaskMS(x) (x / portTICK_PERIOD_MS)

bool wait_for_signal(gpio_num_t, bool);
u32 clock_diff_us(u32);
void print_bits(char buf[], int nbits);

#define HUM_HIGH(x)  (x->byte[0])
#define HUM_LOW(x)   (x->byte[1])
#define TEMP_HIGH(x) (x->byte[2])
#define TEMP_LOW(x)  (x->byte[3])
#define PARITY(x)    (x->byte[4])
struct dht21 {
	u8 byte[5];
};

void dht21_init(gpio_num_t pin)
{
	/*
	 * Configure DHT21 temp/hum sensor
	 */
	gpio_pad_select_gpio(pin);
}

bool dht21_checksum(struct dht21 *data)
{
	u8 hum_high = HUM_HIGH(data);
	u8 hum_low = HUM_LOW(data);
	u8 temp_high = TEMP_HIGH(data);
	u8 temp_low = TEMP_LOW(data);

	u8 sum = hum_high + hum_low + temp_high + temp_low;

#ifdef DBG_CHKSUM
	printf("chk %x %x %x %x == %x real(%x)\n",
			hum_high, hum_low, temp_high, temp_low, PARITY(data), sum);
#endif

	return sum == PARITY(data);
}

/**
 * Polls the DHT21 sensor for humidity and temperature data.
 *
 * The signal pattern below is found in the DHT21 technical reference.
 */
void dht21_poll_data(gpio_num_t pin, struct dht21 *data)
{
	u32 clock_count, diff_us;
	bool signal_error = 0;

	/* Signal READY */
	CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
	gpio_set_level(pin, LOW);
	ets_delay_us(1000);
	gpio_set_level(pin, HIGH);
	ets_delay_us(30);

	/* Enable input and receive OK signal */
	CHECK(gpio_set_direction(pin, GPIO_MODE_INPUT));
	signal_error |= wait_for_signal(pin, LOW);
	signal_error |= wait_for_signal(pin, HIGH);
	signal_error |= wait_for_signal(pin, LOW);

	if (signal_error) {
		LOG("DHT21 initial signal error");
		return;
	}

#ifdef DBG_SIGNAL
	u32 sigs[40];
#endif

	for (i8 bit_received = 0; bit_received < 40; ++bit_received) {
		/* FIXME: error if unexpected signal wait */
		(void) wait_for_signal(pin, HIGH);
		clock_count = xthal_get_ccount();
		(void) wait_for_signal(pin, LOW);
		diff_us = clock_diff_us(clock_count);

		data->byte[bit_received / 8] |= diff_us >= 40 && diff_us <= 80;
		data->byte[bit_received / 8] <<= 1;

#ifdef DBG_SIGNAL
		sigs[39 - bit_pos] = diff_us;
#endif
	}

	/* Reset output signal to high */
	CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
	gpio_set_level(pin, HIGH);

#ifdef DBG_SIGNAL
	printf("Sigs:");
	for (int i = 0; i < 40; ++i)
		printf(" %d", sigs[i]);
	printf("\n");
#endif

#ifdef DBG_CHKSUM
	LOG("checksum=%d", dht21_checksum(data));
#endif
}

void print_bits(char buf[], int nbits)
{
        int i, bit;
        char c;

        //printf("bytes to read: %d\n", bytes_to_read); //DEBUG

        for (i = 0; i < nbits; ++i) {
                c = buf[i];

                for (bit = 0; bit < 8; ++bit) {
                        printf("%i", c & 1);
                        c >>= 1;
                }

                printf("\n");
        }

        printf("\n");
}

/**
 * Returns true if value we wait for is not found
 */
bool wait_for_signal(gpio_num_t pin, bool value)
{
	for (u32 i = 0; i < 100000; ++i) {
		if (gpio_get_level(pin) == value) {
			return false;
		}
	}

	return true;
}

u32 clock_diff_us(u32 last_ccount)
{
	u32 current_ccount;

	/* FIXME: if ccount == 0, no ccount reg */
	current_ccount = xthal_get_ccount();

	// LOG("%u - %u", current_ccount, last_ccount);

	if (current_ccount >= last_ccount) {
		return (current_ccount - last_ccount) / CLOCK_RATE_MHZ;
	} else {
		/* FIXME: account for rollover */
		return 0;
	}
}

void app_main(void)
{
	/* Configure the IOMUX register for pad BLINK_GPIO (some pads are
	   muxed to GPIO on reset already, but some default to other
	   functions and need to be switched to GPIO. Consult the
	   Technical Reference for a list of pads and their default
	   functions.)
	   */
	gpio_pad_select_gpio(BLINK_GPIO);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	struct dht21 sensor_data;
	dht21_init(DHT21_SENSOR_GPIO);
	gpio_set_direction(DHT21_SENSOR_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(DHT21_SENSOR_GPIO, HIGH);
	vTaskDelay(vTaskMS(5000));

	while(1) {
		gpio_set_level(BLINK_GPIO, LOW);

		vTaskDelay(vTaskMS(3000));

		gpio_set_level(BLINK_GPIO, HIGH);

		dht21_poll_data(DHT21_SENSOR_GPIO, &sensor_data);

		vTaskDelay(vTaskMS(3000));
	}
}
