#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "gpio.h"

void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOG);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
}
