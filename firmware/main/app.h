/*
 * app.h
 *
 * Author: cmdc0de
 */

#ifndef CORNCON22_APP_H
#define CORNCON22_APP_H

#include "device/display/display_device.h"
#include "error_type.h"
#include <app/app.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <nvs_memory.h>
#include <freertos.h>
#include <device/display/layout.h>
#include <device/hwbutton/buttonmanager.h>
#include "appconfig.h"

namespace libesp {
class GUI;
class DisplayDevice;
class DisplayMessageState;
class OTA;
};

class MenuState;
class GameOfLife;
class Menu3D;
class SettingMenu;
class BadgeTest;
class MainNav;
class Pacman;
class WiFiMenu;
class ConnectionDetails;
class UpdateMenu;
class HighScore;
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
   enum LEDS {
      ALL_OFF = 0
      , LEFT_ONE = 0x1
      , LEFT_TWO = 0x2
      , LEFT_THREE = 0x8
      , RIGHT_ONE = 0x4
      , RIGHT_TWO = 0x10
      , RIGHT_THREE = 0x20
      , ALL_ON = LEFT_ONE|LEFT_TWO|LEFT_THREE|RIGHT_ONE|RIGHT_TWO|RIGHT_THREE
      , LEFT_ONETWO = LEFT_ONE|LEFT_TWO
      , LEFT_ONETWOTHREE = LEFT_ONE|LEFT_TWO|LEFT_THREE
      , RIGHT_ONETWO = RIGHT_ONE | RIGHT_TWO
      , RIGHT_ONETWOTHREE = RIGHT_ONE|RIGHT_TWO|RIGHT_THREE
   };
  enum MODE {
    ONE,
    TWO
    , THREE
    , FOUR
  };
public:
   typedef typename libesp::ButtonManager<12,2,6,2> BtnManagerType;
	static const char *LOGTAG;
	static const char *MENUHEADER;
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(MyAppMsg*);
	static const char *sYES;
	static const char *sNO;
	static const uint32_t TIME_BETWEEN_PULSES = 200;
	static const uint32_t TIME_BETWEEN_WIFI_CONNECTS = 60000;
  //1.8" TFT
	static const uint16_t DISPLAY_HEIGHT		= 128;
	static const uint16_t DISPLAY_WIDTH			= 160;
	static const uint16_t FRAME_BUFFER_HEIGHT	= 128;
	static const uint16_t FRAME_BUFFER_WIDTH	= 160;

	static const uint32_t ESP_INTR_FLAG_DEFAULT= 0;

	static MyApp &get();
public:
	virtual ~MyApp();
   void setLEDs(MyApp::LEDS l);
	uint16_t getCanvasWidth();
	uint16_t getCanvasHeight();
	uint16_t getLastCanvasWidthPixel();
	uint16_t getLastCanvasHeightPixel();
	libesp::TFTDisplay &getDisplay();
	libesp::GUI &getGUI();
	MenuState *getMenuState();
	SettingMenu *getSettingMenu();
	GameOfLife *getGameOfLife();
	Menu3D *getMenu3D();
   BadgeTest *getBadgeTest();
   MainNav *getMainNavMap();
   WiFiMenu *getWiFiMenu();
   ConnectionDetails *getConnectionDetailMenu();
   UpdateMenu *getUpdateMenu();
   HighScore *getHighScores();
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

#endif 

