#include "badge_test.h"
#include "../app.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include "setting_menu.h"
#include "game_of_life.h"
#include "menu_state.h"
#include <net/ota.h>
#include <freertos.h>
#include <climits> 

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::FreeRTOS;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[BadgeTest::QUEUE_SIZE*BadgeTest::MSG_SIZE] = {0};
const char *BadgeTest::LOGTAG = "BadgeTest";

BadgeTest::BadgeTest() :
	AppBaseMenu(), MenuList("Badge Test", Items, 35, 40, 170,110, -1, ItemCount), ExitTimer(UINT_MAX) {
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
   sprintf(getRow(7),"BadgeID: %s", MyApp::get().getBadgeID());
   sprintf(getRow(8),"Exit hold Bot btn for 2s");
   MyApp::get().getDisplay().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext BadgeTest::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      switch(bme->getButton()) {
      case PIN_NUM_TR_BTN:
         if(bme->wasReleased()) {
            sprintf(getRow(0),"TR: %s", OFF);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_TR_BTN),0);
         } else {
            sprintf(getRow(0),"TR: %s", ON);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_TR_BTN),1);
         }
         break;
      case PIN_NUM_TL_BTN:
         if(bme->wasReleased()) {
            sprintf(getRow(1),"TL: %s", OFF);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_TL_BTN),0);
         } else {
            sprintf(getRow(1),"TL: %s", ON);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_TL_BTN),1);
         }
         break;
      case PIN_NUM_BOT_BTN:
         if(bme->wasReleased()) {
           sprintf(getRow(2),"Bot: %s", OFF);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_BOT_BTN),0);
            ExitTimer = UINT_MAX;
         } else {
           sprintf(getRow(2),"Bot: %s", ON);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_BOT_BTN),1);
            ExitTimer = FreeRTOS::getTimeSinceStart();
         }
         break;
      case PIN_NUM_BR_BTN:
         if(bme->wasReleased()) {
            sprintf(getRow(3),"BR: %s", OFF);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_BR_BTN),0);
         } else {
            sprintf(getRow(3),"BR: %s", ON);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_BR_BTN),1);
         }
         break;
      case PIN_NUM_BL_BTN:
         if(bme->wasReleased()) {
            sprintf(getRow(4),"BL: %s", OFF);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_BL_BTN),0);
         } else {
            sprintf(getRow(4),"BL: %s", ON);
            gpio_set_level(MyApp::get().getLEDForButton(PIN_NUM_BL_BTN),1);
         }
         break;
      default:
         break;
      }
	}
  if(ExitTimer!=UINT_MAX && FreeRTOS::getTimeSinceStart()-ExitTimer>2000) {
    ExitTimer = UINT_MAX;
    nextState = MyApp::get().getMenuState();
  }

   MyApp::get().getDisplay().drawList(&this->MenuList);
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType BadgeTest::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
   MyApp::get().turnOffAllLEDs();
	return ErrorType();
}

