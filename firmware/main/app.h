/*
 * app.h
 *
 * Author: cmdc0de
 */

#pragma once
#include <device/display/GC9A01.h>
#include <device/display/display.h>
#include <error_type.h>
#include <app/app.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <nvs_memory.h>
#include <freertos.h>
#include <device/display/layout.h>
#include <device/hwbutton/buttonmanager.h>
#include "appconfig.h"

namespace libesp {

   template class Display<GC9A01>;
   class DisplayMessageState;
   class OTA;
};


class MenuState;
class GameOfLife;
class Menu3D;
class SettingMenu;
class BadgeTest;
class WiFiMenu;
class ConnectionDetails;
class UpdateMenu;
class PairMenu;
class SleepMenu;

enum ERRORS {
	APP_OK = libesp::ErrorType::APP_OK
	, OTA_INIT_FAIL = libesp::ErrorType::APP_BASE + 1
	, BT_INIT_FAIL
	, WIFI_TASK_INIT_FAIL
};

class MyErrorMap : public libesp::IErrorDetail {
public:
	virtual const char *toString(int32_t err);
	virtual ~MyErrorMap() {}
};

class MyAppMsg;

class MyApp : public libesp::App {
public:
  enum MODE {
    ONE,
    TWO
    , THREE
    , FOUR
  };
public:
   typedef typename libesp::ButtonManager<12,2,5,2> BtnManagerType;
	static const char *LOGTAG;
	static const char *MENUHEADER;
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(MyAppMsg*);
	static const char *sYES;
	static const char *sNO;
	static const uint32_t TIME_BETWEEN_PULSES = 200;
	static const uint32_t TIME_BETWEEN_WIFI_CONNECTS = 60000;

	static const uint16_t DISPLAY_HEIGHT		= 240;
	static const uint16_t DISPLAY_WIDTH			= 240;
	static const uint16_t FRAME_BUFFER_HEIGHT	= 240;
	static const uint16_t FRAME_BUFFER_WIDTH	= 240;

	static const uint32_t ESP_INTR_FLAG_DEFAULT= 0;

	static MyApp &get();
public:
	virtual ~MyApp();
	uint16_t getCanvasWidth();
	uint16_t getCanvasHeight();
	uint16_t getLastCanvasWidthPixel();
	uint16_t getLastCanvasHeightPixel();
	libesp::Display<libesp::GC9A01> &getDisplay();
	MenuState *getMenuState();
	SettingMenu *getSettingMenu();
	GameOfLife *getGameOfLife();
	Menu3D *getMenu3D();
   BadgeTest *getBadgeTest();
   WiFiMenu *getWiFiMenu();
   ConnectionDetails *getConnectionDetailMenu();
   UpdateMenu *getUpdateMenu();
   PairMenu *getPairMenu();
   libesp::OTA &getOTA();
   SleepMenu *getSleepMenu();

   AppConfig &getConfig();
	libesp::DisplayMessageState *getDisplayMessageState(libesp::BaseMenu *, const char *msg, uint32_t msDisplay);
	uint8_t *getBackBuffer();
	uint32_t getBackBufferSize();
	libesp::NVS &getNVS() { return NVSStorage;}
   BtnManagerType &getButtonMgr() {return ButtonMgr;}
   const BtnManagerType &getButtonMgr() const {return ButtonMgr;}
   void goToSleep();
   void wakeUp();
   bool isSleeping() const {return AmISleep;}
protected:
	MyApp();
   libesp::ErrorType initFS();
	libesp::ErrorType initMotionSensor();
	virtual libesp::ErrorType onInit();
	virtual libesp::ErrorType onRun();
private:
	MyErrorMap AppErrors;
	MODE CurrentMode;
	uint32_t LastTime;
	libesp::NVS NVSStorage;
   BtnManagerType ButtonMgr;
   AppConfig Config;
   bool AmISleep;
   uint32_t LastInteractionTime;
private:
	static MyApp mSelf;
};

