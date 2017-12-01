/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include "touch_i2c.h"


void i2c_init(void)
{
	/* Enable clocks for I2C2 and GPIO. */
	rcc_periph_clock_enable(RCC_I2C3);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Set gpio and alternate functions for the SCL and SDA pins of I2C3. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO8);
	gpio_set_af(GPIOC, GPIO_AF4, GPIO9);

	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C3);

        i2c_set_speed(I2C3, i2c_speed_sm_100k, I2C_CR2_FREQ_36MHZ);

	/*
	 * This is our slave address - needed only if we want to receive from
	 * other masters.
	 */
	i2c_set_own_7bit_slave_address(I2C3, 0x32);

	/* If everything is configured -> enable the peripheral. */
	i2c_peripheral_enable(I2C3);
}

