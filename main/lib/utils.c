#include "utils.h"

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


