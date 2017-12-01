/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
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

#include <errno.h>
#include <stdio.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <stdint.h>
#include <math.h>
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "touch_i2c.h"
#include "gfx.h"
#include "gpio.h"

/* Convert degrees to radians */
#define d2r(d) ((d) * 6.2831853 / 360.0)

/*
 * This is our example, the heavy lifing is actually in lcd-spi.c but
 * this drives that code.
 */

#define TEXT_H 11;
int text_y=0;
void my_lcd_print(char *text);
void my_lcd_print(char *text) {
  gfx_setCursor(0, text_y);
  gfx_puts(text);
  text_y=text_y+TEXT_H;
}

int _write(int file, char *ptr, int len)
{
        int i;

        if (file == 1) {
                for (i = 0; i < len; i++) {
                        gfx_write(ptr[i]);
                }
                return i;
        }
        errno = EIO;
        return -1;
}


#define I2C_TOUCH_ADDR 0x41
#define TOUCH_CHIP_ID 0x00
#define TOUCH_ID_VER 0x02
#define TOUCH_SYS_CTRL1 0x03
#define TOUCH_SYS_CTRL2 0x04
#define TOUCH_TSC_CTRL 0x40
#define TOUCH_TSC_CFG 0x41
#define TOUCH_FIFO_TH 0x4A
#define TOUCH_FIFO_STA 0x4B
#define TOUCH_FIFO_SIZE 0x4C
#define TOUCH_TSC_DATA_X 0x4D
#define TOUCH_TSC_DATA_Y 0x4F
#define TOUCH_TSC_DATA_Z 0x51

//For touch screen (origin lower left)
#define MIN_X 350
#define MIN_Y 280
#define MAX_X 3650
#define MAX_Y 3800

// For screen (origin upper left)
#define X_RES 239
#define Y_RES 319

void touch2lcd(int touch_x, int touch_y, int *lcd_x, int *lcd_y) {
  *lcd_x=(touch_x-MIN_X)*X_RES/(MAX_X-MIN_X);
  if (*lcd_x>X_RES) {
    *lcd_x=X_RES;
  }
  if (*lcd_x<0) {
    *lcd_x=0;
  }
  // 0:MAX_Y
  // Y_RES: MIN_Y
  //P = Y_RES+(LCD_Y-MIN_Y)*(0-Y_RES)/(MAX_Y-MIN_Y)
  
  *lcd_y=Y_RES-(touch_y-MIN_Y)*Y_RES/(MAX_Y-MIN_Y);
  if (*lcd_y>Y_RES) {
    *lcd_y=Y_RES;
  }
  if (*lcd_y<0) {
    *lcd_y=0;
  }
}


int main(void)
{

	clock_setup();
        gpio_setup();
	console_setup(115200);
	sdram_init();
	lcd_spi_init();
        i2c_init();
	msleep(1000);
	gfx_init(lcd_draw_pixel, 240, 320);
	gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
	gfx_setTextSize(1);


        gfx_fillScreen(LCD_BLACK);
        uint8_t data2[2];
        uint8_t cmd2[2];
        uint8_t data;
        uint16_t datax, datay, dataz;
        uint8_t cmd;
        uint8_t fifo_state=0;

        cmd = TOUCH_CHIP_ID;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, data2, 2);
        printf("CHIP ID: %X %X\n", data2[0], data2[1]);

        cmd = TOUCH_ID_VER;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("ID VER: %X\n", data);

        cmd = TOUCH_SYS_CTRL2;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("SYS CTRL2: %X\n",  data);

        printf("Disable clocks for everything\n");
        cmd2[0]=TOUCH_SYS_CTRL2;
        cmd2[1]=0x00; //Turn off TSC and ADC clocks
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, cmd2, 2, &data, 0);

        cmd = TOUCH_SYS_CTRL2;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("SYS CTRL2: %X\n",  data);

        cmd = TOUCH_TSC_CTRL;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CTRL: %X\n",  data);

        printf("Configure TSC, disable TSC\n");
        cmd2[0]=TOUCH_TSC_CTRL;
        cmd2[1]=0x11; //Turn on TSC, 4 tracking index, TSC disable
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, cmd2, 2, &data, 0);

        cmd = TOUCH_TSC_CTRL;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CTRL: %X\n",  data);

        cmd = TOUCH_TSC_CFG;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CFG: %X\n",  data);

        printf("Configure CFG, 4 samples, delay 500ms, settling 500us\n");
        cmd2[0]=TOUCH_TSC_CFG;
        cmd2[1]=0x9A; 
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, cmd2, 2, &data, 0);

        cmd = TOUCH_TSC_CFG;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CFG: %X\n",  data);

        cmd = TOUCH_FIFO_TH;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("FIFO TH: %X\n",  data);

        printf("Configure FIFO TH, 0xFF\n");
        cmd2[0]=TOUCH_FIFO_TH;
        cmd2[1]=0x1; //Not zero!
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, cmd2, 2, &data, 0);

        cmd = TOUCH_FIFO_TH;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("FIFO TH: %X\n",  data);

        cmd = TOUCH_SYS_CTRL2;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("SYS CTRL2: %X\n",  data);

        printf("Enable clocks for everything\n");
        cmd2[0]=TOUCH_SYS_CTRL2;
        cmd2[1]=0x00; //Turn on all clocks
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, cmd2, 2, &data, 0);

        cmd = TOUCH_SYS_CTRL2;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("SYS CTRL2: %X\n",  data);

        cmd = TOUCH_TSC_CTRL;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CTRL: %X\n",  data);

        printf("Configure TSC, enable TSC\n");
        cmd2[0]=TOUCH_TSC_CTRL;
        cmd2[1]=0x11; //Turn on TSC, 4 tracking index, TSC disable
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, cmd2, 2, &data, 0);

        cmd = TOUCH_TSC_CTRL;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CTRL: %X\n",  data);

        cmd = TOUCH_TSC_CFG;
        i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
        printf("TSC CFG: %X\n",  data);

        lcd_show_frame();
        //while (1) {};
	msleep(3000);

        int x=0;
        int y=0;

	while (1) {
          text_y=0;
          gfx_setCursor(0, 0);
          gfx_fillScreen(LCD_BLACK);

          cmd = TOUCH_FIFO_STA;
          i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &data, 1);
          printf("FIFO STA: %X\n",  data);

          cmd = TOUCH_FIFO_SIZE;
          i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &fifo_state, 1);
          printf("FIFO SIZE: %X\n",  fifo_state);

          while(fifo_state>0) {

            cmd = TOUCH_TSC_DATA_X;
            i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, data2, 2);
            datax=data2[1]|(data2[0]<<8);

            cmd = TOUCH_TSC_DATA_Y;
            i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, data2, 2);
            datay=data2[1]|(data2[0]<<8);

            cmd = TOUCH_TSC_DATA_Z;
            i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, data2, 2);
            dataz=data2[1];

            cmd = TOUCH_FIFO_SIZE;
            i2c_transfer7(I2C3, I2C_TOUCH_ADDR, &cmd, 1, &fifo_state, 1);


          }

          printf("DATA X: %d\n",  datax);
          printf("DATA Y: %d\n",  datay);
          printf("DATA Z: %d\n",  dataz);
          printf("FIFO SIZE: %X\n",  fifo_state);

          touch2lcd(datax, datay, &x, &y);
          printf("X %d, Y %d\n", x, y);

          gfx_drawPixel(x, y, LCD_WHITE);

          gpio_set(GPIOG, GPIO13);	/* LED on/off */
          lcd_show_frame();
          gpio_clear(GPIOG, GPIO13);	/* LED on/off */
          //for (int i = 0; i < 1000000; i++) {	/* Wait a bit. */
          //  __asm__("nop");
          //}
	}
}
