#include "badge_test.h"
#include "../app.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include "setting_menu.h"
#include "game_of_life.h"
#include "menu_state.h"
#include <net/ota.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[BadgeTest::QUEUE_SIZE*BadgeTest::MSG_SIZE] = {0};
const char *BadgeTest::LOGTAG = "BadgeTest";

BadgeTest::BadgeTest() :
	AppBaseMenu(), MenuList("Badge Test", Items, 35, 40, 170,110, -1, ItemCount) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

BadgeTest::~BadgeTest() {

}

static const constexpr char * OFF = "OFF";
static const constexpr char * ON = "ON";

ErrorType BadgeTest::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   for(int i=0;i<ItemCount;++i) {
      Items[i].id = i;
	   Items[i].text = getRow(i);
   }
   sprintf(getRow(0),"TR: %s", OFF);
   sprintf(getRow(1),"TL: %s", OFF);
   sprintf(getRow(2),"Bot: %s", OFF);
   sprintf(getRow(3),"BR: %s", OFF);
   sprintf(getRow(4),"BL: %s", OFF);
   sprintf(getRow(5),"Version: %s", MyApp::get().getOTA().getCurrentApplicationVersion());
   sprintf(getRow(6),"Build Date: %s", MyApp::get().getOTA().getBuildDate());
   sprintf(getRow(7),"BadgeID: %s", "XXX");
   MyApp::get().getDisplay().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext BadgeTest::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      switch(bme->getButton()) {
      case PIN_NUM_UP_BTN:
         if(bme->wasReleased()) sprintf(getRow(0),"TR: %s", OFF);
         else sprintf(getRow(0),"TR: %s", ON);
         break;
      case PIN_NUM_DOWN_BTN:
         if(bme->wasReleased()) sprintf(getRow(1),"TL: %s", OFF);
         else sprintf(getRow(1),"TL: %s", ON);
         break;
      case PIN_NUM_LEFT_BTN:
         if(bme->wasReleased()) sprintf(getRow(2),"Bot: %s", OFF);
         else sprintf(getRow(2),"Bot: %s", ON);
         break;
      case PIN_NUM_RIGHT_BTN:
         if(bme->wasReleased()) {
            sprintf(getRow(3),"BR: %s", OFF);
         } else {
            sprintf(getRow(3),"BR: %s", ON);
         }
         break;
      case PIN_NUM_FIRE_BTN:
         if(bme->wasReleased()) sprintf(getRow(4),"BL: %s", OFF);
         else sprintf(getRow(4),"BL: %s", ON);
         //nextState = MyApp::get().getMenuState();
         break;
      default:
         break;
      }
	}

   MyApp::get().getDisplay().drawList(&this->MenuList);
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType BadgeTest::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
	return ErrorType();
}

