#pragma once

#include <types.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define HIGH 1
#define LOW 0

#define LOG(fmt, ...) printf("LOG: " fmt "\n", ##__VA_ARGS__)

#define CHECK(code) if (code != ESP_OK) { LOG("ESP_ERR: %s", #code); }

#define CLOCK_RATE_MHZ (240)
#define CLOCK_RATE_HZ (CLOCK_RATE_MHZ * 1000000)

/**
 * Print bits in char buffer
 */
void print_bits(char buf[], int nbits);

/**
 * Returns true if value we wait for is not found
 */
bool wait_for_signal(gpio_num_t pin, bool value);

/**
 * Return clock diff in microseconds
 */
u32 clock_diff_us(u32 last_ccount);

