#include "menu_state.h"
#include "../app.h"
#include "gui_list_processor.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include <math/rectbbox.h>
#include "setting_menu.h"
#include "game_of_life.h"
#include "badge_test.h"
#include "main_nav.h"
#include "wifi_menu.h"
#include "connection_details.h"
#include "high_score.h"
#include "pair.h"
#include "menu3d.h"
#include "sleep_menu.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::Point2Ds;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MenuState::QUEUE_SIZE*MenuState::MSG_SIZE] = {0};

MenuState::MenuState() :
	AppBaseMenu(), MenuList("Main Menu", Items, 0, 0, MyApp::get().getCanvasWidth(), MyApp::get().getCanvasHeight(), 0, ItemCount) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

MenuState::~MenuState() {

}

ErrorType MenuState::onInit() {
   MyApp::get().setLEDs(MyApp::LEDS::ALL_OFF);
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   Items[0].id = 0;
	if (MyApp::get().getConfig().isNameSet()) {
		Items[0].text = (const char *) "Settings";
	} else {
		Items[0].text = (const char *) "Settings *";
	}
	Items[1].id = 1;
	Items[1].text = (const char *) "Screen Saver";
	Items[2].id = 2;
	Items[2].text = (const char *) "Test Badge";
	Items[3].id = 3;
	Items[3].text = (const char *) "3D Cube";
	Items[4].id = 4;
	Items[4].text = (const char *) "Invaders";
   Items[5].id = 5;
   Items[5].text = "High Scores";
   Items[6].id = 6;
   if (MyApp::get().getWiFiMenu()->isConnected()) Items[6].text = (const char *) "WiFi (Connected)";
   else Items[6].text = (const char *) "WiFi (NOT Connected)";
   Items[7].id = 7;
   Items[7].text = "Connection Details";
   Items[8].id = 8;
   Items[8].text = "Initiate Pair";
   Items[9].id = 9;
   Items[9].text = "Pair";
   Items[10].id = 10;
   Items[10].text = "Sleep";
   MyApp::get().getGUI().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext MenuState::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
   bool wasFireBtnReleased = false;
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      if(bme->wasReleased()) {
         switch(bme->getButton()) {
            case PIN_NUM_FIRE_BTN:
               wasFireBtnReleased = true;
            break;
            case PIN_NUM_UP_BTN:
               MenuList.moveUp();
            break;
            case PIN_NUM_DOWN_BTN:
               MenuList.moveDown();
            break;
            case PIN_NUM_LEFT_BTN:
               MenuList.selectTop();
            break;
            default:
            break;
         }
      }
	}

   //char buf[32] = {'\0'};
   if(wasFireBtnReleased) {
      switch (MenuList.selectedItem) {
      case 0:
         nextState = MyApp::get().getSettingMenu();
         break;
      case 1:
         nextState = MyApp::get().getGameOfLife();
         break;
      case 2:
         nextState = MyApp::get().getBadgeTest();
         break;
      case 3:
         //nextState = MyApp::get().getMainNavMap();
         nextState = MyApp::get().getMenu3D();
         break;
      case 4:
         //nextState = MyApp::get().getPacman();
         //nextState = MyApp::get().getInvaders();
         break;
      case 5:
         nextState = MyApp::get().getHighScores();
         break;
      case 6:
         nextState = MyApp::get().getWiFiMenu();
         break;
      case 7:
         nextState = MyApp::get().getConnectionDetailMenu();
         break;
      case 8:
         MyApp::get().getPairMenu()->initatePair(true);
         nextState = MyApp::get().getPairMenu();
         break;
      case 9:
         MyApp::get().getPairMenu()->initatePair(false);
         nextState = MyApp::get().getPairMenu();
         break;
      case 10:
         nextState = MyApp::get().getSleepMenu();
         break;
      }
   }
   if (MyApp::get().getWiFiMenu()->isConnected()) Items[6].text = (const char *) "WiFi (Connected)";
   else Items[6].text = (const char *) "WiFi (NOT Connected)";
    
   MyApp::get().getGUI().drawList(&this->MenuList);
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MenuState::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
	return ErrorType();
}

