#ifndef PINCONFIG_H
#define PINCONFIG_H

#include <system.h>

#define PIN_NUM_BTN_1				GPIO_NUM_26
#define PIN_NUM_BTN_2				GPIO_NUM_27
#define PIN_NUM_BTN_3				GPIO_NUM_14
#define PIN_NUM_BTN_4				GPIO_NUM_12

#define PIN_NUM_UP_BTN           PIN_NUM_BTN_1
#define PIN_NUM_DOWN_BTN         PIN_NUM_BTN_2
#define PIN_NUM_FIRE_BTN         PIN_NUM_BTN_3
#define PIN_NUM_JUMP_BTN         PIN_NUM_BTN_4
#define PIN_NUM_LEFT_BTN         PIN_NUM_FIRE_BTN
#define PIN_NUM_RIGHT_BTN        PIN_NUM_JUMP_BTN

//display
#define PIN_NUM_DISPLAY_BACKLIGHT 		GPIO_NUM_4
#define PIN_NUM_DISPLAY_DATA_CMD  		GPIO_NUM_5
#define PIN_NUM_DISPLAY_CS        		GPIO_NUM_18
#define PIN_NUM_SD_CS  			      	GPIO_NUM_15
#define PIN_NUM_DISPLAY_SCK       		GPIO_NUM_16
#define PIN_NUM_DISPLAY_MISO      		GPIO_NUM_25
#define PIN_NUM_DISPLAY_MOSI      		GPIO_NUM_17
#define PIN_NUM_DISPLAY_RESET     		NOPIN

//LEDS
#define PIN_NUM_LED_CLK				GPIO_NUM_23
#define PIN_NUM_LED_DATA			GPIO_NUM_22


#endif
