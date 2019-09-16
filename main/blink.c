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

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define HIGH 1
#define LOW 0

#define LOG(fmt, ...) printf("LOG: " #fmt "\n", ##__VA_ARGS__)

#define CHECK(code) if (code != ESP_OK) { LOG("ESP_ERR: %s", #code); }

/* 40 MHz crystal */
#define CLOCK_RATE_MHZ (240)
#define CLOCK_RATE_HZ (CLOCK_RATE_MHZ * 1000000)

#define BLINK_GPIO GPIO_NUM_2
#define DHT21_SENSOR_GPIO GPIO_NUM_25

#define vTaskMS(x) (x / portTICK_PERIOD_MS)

bool wait_for_signal(gpio_num_t, bool);
u32 clock_diff_us(u32);

struct dht21_data {
	uint16_t humidity;
	uint16_t temperature;
	uint8_t _parity;
};

void dht21_init(gpio_num_t pin)
{
	/*
	 * Configure DHT21 temp/hum sensor
	 */
	gpio_pad_select_gpio(pin);
}

/**
 * Polls the DHT21 sensor for humidity and temperature data.
 *
 * The signal pattern below is found in the DHT21 technical reference.
 */
void dht21_poll_data(gpio_num_t pin, struct dht21_data *data)
{
	u32 clock_count, diff_us;
	bool signal_error = 0;

	/* Signal READY */
	CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
	gpio_set_level(pin, LOW);
	// vTaskDelay(vTaskMS(1));
	ets_delay_us(1000);
	gpio_set_level(pin, HIGH);
	clock_count = xthal_get_ccount();
	ets_delay_us(30);
	diff_us = clock_diff_us(clock_count);
	LOG("MY CLOCK HIGH %u us", diff_us);

	/* Enable input and receive OK signal */
	CHECK(gpio_set_direction(pin, GPIO_MODE_INPUT));
	signal_error |= wait_for_signal(pin, LOW);
	clock_count = xthal_get_ccount();
	signal_error |= wait_for_signal(pin, HIGH);
	diff_us = clock_diff_us(clock_count);
	LOG("OK SIGNAL HIGH %u us", diff_us);
	signal_error |= wait_for_signal(pin, LOW);

	if (signal_error) {
		LOG("DHT21 initial signal error");
		return;
	}

	for (u8 bit_num = 39; bit_num > 0; --bit_num) {
		/* FIXME: error if unexpected signal wait */
		(void) wait_for_signal(pin, HIGH);
		clock_count = xthal_get_ccount();
		(void) wait_for_signal(pin, LOW);
		diff_us = clock_diff_us(clock_count);

		if (diff_us <= 30) {
			/* got 0 */
			LOG("Got 0");
		} else if (diff_us <= 75) {
			/* got 1 */
			LOG("Got 1");
		} else {
			/* erroneous signal */
			// LOG("Erroneous signal! us = %u", diff_us);
		}
	}

	/* Reset output signal to high */
	CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
	gpio_set_level(pin, HIGH);
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

	struct dht21_data sensor_data;
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
