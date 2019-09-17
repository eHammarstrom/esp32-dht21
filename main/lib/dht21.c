#include "dht21.h"
#include "utils.h"
#include "driver/gpio.h"

void dht21_init(gpio_num_t pin)
{
	/*
	 * Configure DHT21 temp/hum sensor
	 */
	gpio_pad_select_gpio(pin);
}

bool dht21_checksum_ok(struct dht21 *data)
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

i16 dht21_temperature(struct dht21 *data)
{
	i16 temperature;
	bool negate;

	/* check negation bit */
	negate = TEMP_HIGH(data) & 0x80;

	temperature = ((TEMP_HIGH(data) & 0x7F) * 256) + // 0x7F00 * 256 without negation bit
		((TEMP_LOW(data) & 0xF0) >> 4) * 16 +    // 0x00F0 * 16
		(TEMP_LOW(data) & 0x0F);                 // 0x000F * 1

	print_bits((char *) data->byte + 2, 2);

	return negate ? -temperature : temperature;
}

u16 dht21_humidity(struct dht21 *data)
{
	u16 humidity;

	humidity = HUM_HIGH(data) * 256 +            // 0xFF00 * 256
		((HUM_LOW(data) & 0xF0) >> 4) * 16 + // 0x00F0 * 16
		(HUM_LOW(data) & 0x0F);              // 0x000F * 1


	return humidity;
}

