#ifndef PINCONFIG_H
#define PINCONFIG_H

#include <system.h>

#define PIN_NUM_BTN_1				GPIO_NUM_23
#define PIN_NUM_BTN_2				GPIO_NUM_26
#define PIN_NUM_BTN_3				GPIO_NUM_32
#define PIN_NUM_BTN_4				GPIO_NUM_33
#define PIN_NUM_BTN_5				GPIO_NUM_25

#define PIN_NUM_UP_BTN           PIN_NUM_BTN_1
#define PIN_NUM_DOWN_BTN         PIN_NUM_BTN_2
#define PIN_NUM_FIRE_BTN         PIN_NUM_BTN_3
#define PIN_NUM_JUMP_BTN         PIN_NUM_BTN_4
#define PIN_NUM_LEFT_BTN         PIN_NUM_FIRE_BTN
#define PIN_NUM_RIGHT_BTN        PIN_NUM_BTN_5

//display
#define PIN_NUM_DISPLAY_DATA_CMD  		GPIO_NUM_5
#define PIN_NUM_DISPLAY_CS        		GPIO_NUM_18
#define PIN_NUM_DISPLAY_SCK       		GPIO_NUM_16
#define PIN_NUM_DISPLAY_MISO      		NOPIN
#define PIN_NUM_DISPLAY_MOSI      		GPIO_NUM_17
#define PIN_NUM_DISPLAY_RESET     		GPIO_NUM_19
#define PIN_NUM_DISPLAY_BACKLIGHT 		NOPIN

//LEDS
#define PIN_NUM_LED_1				GPIO_NUM_21
#define PIN_NUM_LED_2				GPIO_NUM_27
#define PIN_NUM_LED_3				GPIO_NUM_14
#define PIN_NUM_LED_4				GPIO_NUM_12
#define PIN_NUM_LED_5				GPIO_NUM_13


#endif
