/*
 * app.cpp
 *
 * Author: cmdc0de
 */


#include "app.h"
#include <esp_log.h>
#include <system.h>
#include <spibus.h>

#include <driver/uart.h>
#include <driver/gpio.h>
#include <device/display/fonts.h>
#include <device/touch/XPT2046.h>
#include <device/display/GC9A01.h>
#include "device/display/basic_back_buffer.h"
#include "device/display/color.h"
#include "device/display/display_types.h"
#include <device/display/display.h>
#include "hal/gpio_types.h"
#include "hal/spi_types.h"
#include "menus/menu_state.h"
#include "menus/game_of_life.h"
#include "menus/setting_menu.h"
#include "menus/menu3d.h"
#include <app/display_message_state.h>
#include "spibus.h"
#include "freertos.h"
#include "fatfsvfs.h"
#include "pinconfig.h"
#include <math/point.h>
#include <esp_spiffs.h>
#include <device/shiftregister/software_shift.h>
#include "appconfig.h"
#include "menus/badge_test.h"
#include "menus/wifi_menu.h"
#include "menus/connection_details.h"
#include <net/ota.h>
#include "menus/update_menu.h"
#include "menus/pair.h"
#include "menus/sleep_menu.h"
#include <esp_partition.h>

using libesp::ErrorType;
using libesp::System;
using libesp::FreeRTOS;
using libesp::RGBColor;
using libesp::SPIBus;
using libesp::Display;
using libesp::DisplayMessageState;
using libesp::BaseMenu;
using libesp::Point2Ds;

const char *MyApp::LOGTAG = "AppTask";
const char *MyApp::sYES = "Yes";
const char *MyApp::sNO = "No";

uint16_t BkBuffer[MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT];
uint16_t *BackBuffer = &BkBuffer[0];

libesp::BasicBackBuffer FrameBuf(MyApp::FRAME_BUFFER_WIDTH, MyApp::FRAME_BUFFER_HEIGHT, uint8_t(16)
      , (uint8_t *)&BkBuffer[0], MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT*2
      , libesp::LIB_PIXEL_FORMAT::FORMAT_16_BIT);


libesp::GC9A01 MyDisplayType(libesp::PORTAIT_TOP_LEFT);

libesp::Display<libesp::GC9A01> MyDisplay;

WiFiMenu MyWiFiMenu;

const char *UPDATE_URL = "https://s3.us-west-2.amazonaws.com/online.corncon.badge/2023/corncon23.bin";
libesp::OTA CCOTA;

const char *MyErrorMap::toString(int32_t err) {
	return "TODO";
}

MyApp MyApp::mSelf;
static MyApp::BtnManagerType::ButtonInfo SButtonInfo[] = {
  {PIN_NUM_BTN_1,false}
  ,{PIN_NUM_BTN_2,false}
  ,{PIN_NUM_BTN_3,false}
  ,{PIN_NUM_BTN_4,false}
  ,{PIN_NUM_BTN_5,false}
};

MyApp &MyApp::get() {
	return mSelf;
}

MyApp::MyApp() : AppErrors(), CurrentMode(ONE), LastTime(0), NVSStorage("appdata","data",false)
                 , ButtonMgr(true), Config(&NVSStorage), AmISleep(false), LastInteractionTime(0) {
	ErrorType::setAppDetail(&AppErrors);
}

MyApp::~MyApp() {

}

AppConfig &MyApp::getConfig() {
   return Config;
}

uint8_t *MyApp::getBackBuffer() {
	return (uint8_t *)&BackBuffer[0];
}

uint32_t MyApp::getBackBufferSize() {
	return MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT*2;
}
  
ErrorType MyApp::initFS() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/www",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LOGTAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LOGTAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(LOGTAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(LOGTAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(LOGTAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

void MyApp::goToSleep() {
   AmISleep = true;
   getDisplay().setBacklight(0);
   getWiFiMenu()->disconnect();
}

void MyApp::wakeUp() {
   AmISleep = false;
   getDisplay().setBacklight(100);
   getWiFiMenu()->connect();
}

libesp::ErrorType MyApp::onInit() {
	ErrorType et;

   ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

   //init leds
   //zero-initialize the config structure.
   ESP_LOGI(LOGTAG,"config gpio for leds");
   gpio_config_t io_conf = {};
   //disable interrupt
   io_conf.intr_type = GPIO_INTR_DISABLE;
   //set as output mode
   io_conf.mode = GPIO_MODE_OUTPUT;
   //bit mask of the pins that you want to set,e.g.GPIO18/19
   io_conf.pin_bit_mask = (1ULL<<PIN_NUM_LED_1) | (1ULL<<PIN_NUM_LED_2) | (1ULL<<PIN_NUM_LED_3) | (1ULL<<PIN_NUM_LED_4) | (1ULL << PIN_NUM_LED_5);
    //disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    //disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //configure GPIO with the given settings
   gpio_config(&io_conf);
   ESP_LOGI(LOGTAG,"***************turn on leds");
   int32_t ledState = 1;
   for(int i=0;i<2;++i) {
      gpio_set_level(PIN_NUM_LED_1,ledState);
		vTaskDelay(500 / portTICK_RATE_MS);
      gpio_set_level(PIN_NUM_LED_2,ledState);
		vTaskDelay(500 / portTICK_RATE_MS);
      gpio_set_level(PIN_NUM_LED_3,ledState);
		vTaskDelay(500 / portTICK_RATE_MS);
      gpio_set_level(PIN_NUM_LED_4,ledState);
		vTaskDelay(500 / portTICK_RATE_MS);
      gpio_set_level(PIN_NUM_LED_5,ledState);
      vTaskDelay(500 / portTICK_RATE_MS);
      ledState = ledState == 1 ? 0: 1;
   }
   //

   CCOTA.init(UPDATE_URL);
   CCOTA.logCurrentActiveParitionInfo();
   if(CCOTA.isUpdateAvailable()) {
      ESP_LOGI(LOGTAG,"*****UPDATE AVAILABLE!!!****");
      et = CCOTA.applyUpdate(true);
      if(et.ok()) {
         ESP_LOGI(LOGTAG,"UPDATE SUCCESSFUL to version %s",CCOTA.getCurrentApplicationVersion());
      } else {
         ESP_LOGI(LOGTAG,"UPDATE FAILED");
      }
   }
	
   ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());


  //initFS();
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	et = NVSStorage.init();
	if(!et.ok()) {
		ESP_LOGI(LOGTAG, "1st InitNVS failed bc %s\n", et.toString());
		et = NVSStorage.initStorage();
		if(et.ok()) {
      ESP_LOGI(LOGTAG, "initStorage succeeded");
			et = NVSStorage.init();
      if(et.ok()) {
        ESP_LOGI(LOGTAG, "NVSSTorage init successful");
      } else {
		    ESP_LOGE(LOGTAG, "2nd InitNVS failed bc %s\nTHIS IS A PROBLEM\n", et.toString());
      }
		} else {
			ESP_LOGI(LOGTAG, "initStorage failed %s\n", et.toString());
		}
	}
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

   et = Config.init();
   if(!et.ok()) {
      ESP_LOGI(LOGTAG,"failed to init config store");
   }

   et = libesp::GC9A01::spiInit(PIN_NUM_DISPLAY_MOSI, PIN_NUM_DISPLAY_MISO, PIN_NUM_DISPLAY_SCK, 1, SPI3_HOST);

   if(!et.ok()) {
      ESP_LOGE(LOGTAG,"Failed to init SPI bus for display");
   }
  
   SPIBus *hbus = libesp::SPIBus::get(SPI3_HOST);

   et = MyDisplayType.init(hbus, PIN_NUM_DISPLAY_CS, PIN_NUM_DISPLAY_DATA_CMD, PIN_NUM_DISPLAY_RESET
         , PIN_NUM_DISPLAY_BACKLIGHT, &FrameBuf, nullptr);

   if(!et.ok()) {
      ESP_LOGE(LOGTAG,"Failed to init SPI bus for display");
   }

   et = MyDisplay.init(&MyDisplayType, &FrameBuf, &Font_6x10, RGBColor::WHITE, RGBColor::BLACK);
   MyDisplay.setRotation(libesp::PORTAIT_TOP_LEFT);

   ESP_LOGI(LOGTAG,"After Display: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

  if(et.ok()) {
		ESP_LOGI(LOGTAG,"display init OK");
		getDisplay().fillScreen(libesp::RGBColor::BLACK);
		getDisplay().swap();
#if 0
		ESP_LOGI(LOGTAG,"******************black");
      for(int i=0;i<239;++i) {
         getDisplay().drawHorizontalLine(0  , i, 239, RGBColor::WHITE);
		   getDisplay().swap();
      }
      for(int i=238;i>=0;--i) {
         getDisplay().drawHorizontalLine(0  , i, 239, RGBColor::RED);
		   getDisplay().swap();
      }
		vTaskDelay(1000 / portTICK_RATE_MS);
		getDisplay().fillScreen(libesp::RGBColor::BLACK);
		getDisplay().swap();
      getDisplay().fillRec(40, 100, 180, 20, RGBColor::GREEN);
      getDisplay().fillRec(40, 130, 60, 20, RGBColor::BLUE);
		getDisplay().swap();
		ESP_LOGI(LOGTAG,"******************rec done do lines");
		vTaskDelay(2000 / portTICK_RATE_MS);
		getDisplay().fillScreen(libesp::RGBColor::BLACK);
      getDisplay().drawVerticalLine(120,   0, 240, RGBColor::WHITE);
      getDisplay().drawHorizontalLine(0  , 120, 240, RGBColor::WHITE);
		getDisplay().swap();
		vTaskDelay(2000 / portTICK_RATE_MS);
#endif
		getDisplay().drawString(70,80,"CornCorn '23",libesp::RGBColor::GREEN, libesp::RGBColor::BLACK,1,false);
		getDisplay().swap();
		ESP_LOGI(LOGTAG,"string");
		vTaskDelay(2000 / portTICK_RATE_MS);
	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}

   ButtonMgr.init(&SButtonInfo[0],true);
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	//vTaskDelay(2000 / portTICK_RATE_MS);
   //MyWiFiMenu.initWiFi();
   //if(getConfig().hasWiFiBeenSetup().ok()) {
    //  et = MyWiFiMenu.connect();
  //		setCurrentMenu(getMenuState());
   //} else {
      //ESP_LOGI(LOGTAG,"Wifi config not set");
      setCurrentMenu(getMenuState());
   //}

   return et;
}

ErrorType MyApp::onRun() {
   ErrorType et;
   ButtonMgr.poll(); //move away from poll 
   int32_t interactionCount = ButtonMgr.broadcast();
   libesp::BaseMenu::ReturnStateContext rsc = getCurrentMenu()->run();
	getDisplay().swap();
   uint32_t now = FreeRTOS::getTimeSinceStart();
   if(LastInteractionTime==0 || interactionCount>0) {
      LastInteractionTime = now;
   }

	if (rsc.Err.ok()) {
      if((now-LastInteractionTime)>(getConfig().getSleepMin()*60*1000)) {
         setCurrentMenu(getSleepMenu());
      } else if (getCurrentMenu() != rsc.NextMenuToRun) {
			setCurrentMenu(rsc.NextMenuToRun);
			ESP_LOGI(LOGTAG,"on Menu swap: Free: %u, Min %u",
				System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
		} else {
		}
	} 

  uint32_t timeSinceLast = now-LastTime;

  if(timeSinceLast>=TIME_BETWEEN_PULSES) {
    LastTime = FreeRTOS::getTimeSinceStart();

    switch(CurrentMode) {
    case ONE:
      {
         CurrentMode = TWO;
      }
      break;
    case TWO:
      {
        CurrentMode = THREE;
      }
      break;
    case THREE:
      {
        // setLEDs(MyApp::LEDS::ALL_OFF);
        CurrentMode = FOUR;
      }
      break;
    case FOUR:
      {
      }
      break;
    }
  }
	return et;
}

uint16_t MyApp::getCanvasWidth() {
	return FrameBuf.getBufferWidth(); 
}

uint16_t MyApp::getCanvasHeight() {
	return FrameBuf.getBufferHeight();
}

uint16_t MyApp::getLastCanvasWidthPixel() {
	return getCanvasWidth()-1;
}

uint16_t MyApp::getLastCanvasHeightPixel() {
	return getCanvasHeight()-1;
}

libesp::Display<libesp::GC9A01> &MyApp::getDisplay() {
   return MyDisplay;
}

MenuState MyMenuState;
libesp::DisplayMessageState DMS;
SettingMenu MySettingMenu;
GameOfLife GOL;
Menu3D Menu3DRender( uint8_t(float(MyApp::FRAME_BUFFER_WIDTH)*0.8f) , uint8_t(float(MyApp::FRAME_BUFFER_HEIGHT)*0.8f));
BadgeTest BadgeTestMenu;
ConnectionDetails MyConDetails;
UpdateMenu MyUpdateMenu;
PairMenu MyPairMenu;
SleepMenu MySleepMenu;

SleepMenu *MyApp::getSleepMenu() {
   return &MySleepMenu;
}


PairMenu *MyApp::getPairMenu() {
   return &MyPairMenu;
}

UpdateMenu *MyApp::getUpdateMenu() {
   return &MyUpdateMenu;
}

ConnectionDetails *MyApp::getConnectionDetailMenu() {
   return &MyConDetails;
}

Menu3D *MyApp::getMenu3D() {
  return &Menu3DRender;
}

GameOfLife *MyApp::getGameOfLife() {
  return &GOL;
}

MenuState *MyApp::getMenuState() {
	return &MyMenuState;
}

SettingMenu *MyApp::getSettingMenu() {
	return &MySettingMenu;
}

WiFiMenu *MyApp::getWiFiMenu() {
   return &MyWiFiMenu;
}


BadgeTest *MyApp::getBadgeTest() {
   return &BadgeTestMenu;
}

libesp::OTA &MyApp::getOTA() {
   return CCOTA;
}


DisplayMessageState *MyApp::getDisplayMessageState(BaseMenu *bm, const char *msg, uint32_t msDisplay) {
	DMS.setMessage(msg);
	DMS.setNextState(bm);
	DMS.setTimeInState(msDisplay);
	//DMS.setDisplay(&Display);
	return &DMS;
}

