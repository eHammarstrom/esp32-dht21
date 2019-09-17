/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
   */
#include <stdio.h>
#include <types.h>
#include <utils.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "lib/dht21.h"

#define BLINK_GPIO GPIO_NUM_2
#define DHT21_SENSOR_GPIO GPIO_NUM_25

#define vTaskMS(x) (x / portTICK_PERIOD_MS)

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
