#include "menu_state.h"
#include "../app.h"
#include "gui_list_processor.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include <math/rectbbox.h>
#include "setting_menu.h"
#include "game_of_life.h"
#include "badge_test.h"
#include "wifi_menu.h"
#include "connection_details.h"
#include "menu3d.h"
#include "sleep_menu.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::Point2Ds;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MenuState::QUEUE_SIZE*MenuState::MSG_SIZE] = {0};

MenuState::MenuState() :
	AppBaseMenu(), MenuList("Main Menu", Items, 35, 40, 170, 110, 0, ItemCount) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

MenuState::~MenuState() {

}

ErrorType MenuState::onInit() {
   clearListBuffer();
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   for(int i=0;i<ItemCount;++i) {
      Items[i].id = i;
	   Items[i].text = getRow(i);
   }
	if (MyApp::get().getConfig().isNameSet()) {
      sprintf(getRow(0),"Settings");
	} else {
      sprintf(getRow(0),"Settings *");
	}
   sprintf(getRow(1), "Screen Saver");
   sprintf(getRow(2), "Test Badge");
   sprintf(getRow(3), "3D Cube");
   sprintf(getRow(4), "Simnon Says: Solo");
   sprintf(getRow(5), "Simon Says: Multiplayer"); 
   if (MyApp::get().getWiFiMenu()->isConnected()) {
      sprintf(getRow(6), "WiFi (Connected)"); 
   } else {
      sprintf(getRow(6), "WiFi (NOT Connected)"); 
   }
   sprintf(getRow(7), "Connection Details");
   sprintf(getRow(8), "Sleep");
   MyApp::get().getDisplay().drawList(&this->MenuList);
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
        case PIN_NUM_SELECT_BTN:
          wasFireBtnReleased = true;
          break;
        case PIN_NUM_TL_BTN:
          MenuList.moveUp();
          break;
        case PIN_NUM_BL_BTN:
          MenuList.moveDown();
          break;
        case PIN_NUM_TR_BTN:
          MenuList.selectTop();
          break;
        default:
          break;
      }
    }
	}

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
         nextState = MyApp::get().getMenu3D();
      break;
      case 4:
         nextState = MyApp::get().getSimonSaysMenu();
      break;
      case 5:
         nextState = MyApp::get().getSimonSaysMenu();
      break;
      case 6:
         nextState = MyApp::get().getWiFiMenu();
      break;
      case 7:
         nextState = MyApp::get().getConnectionDetailMenu();
      break;
      case 8:
         nextState = MyApp::get().getSleepMenu();
      break;
      }
   }
   if (MyApp::get().getWiFiMenu()->isConnected()) {
      sprintf(getRow(6), "WiFi (Connected)"); 
   } else {
      sprintf(getRow(6), "WiFi (NOT Connected)"); 
   }
    
   MyApp::get().getDisplay().drawList(&this->MenuList);
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MenuState::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
	return ErrorType();
}

