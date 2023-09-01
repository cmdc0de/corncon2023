#include "badge_test.h"
#include "../app.h"
#include "gui_list_processor.h"
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
	AppBaseMenu(), MenuList("Badge Test (jump+right)", Items, 0, 0, MyApp::get().getCanvasWidth(), MyApp::get().getCanvasHeight(), 0, ItemCount) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

BadgeTest::~BadgeTest() {

}

static const constexpr char * OFF = "OFF";
static const constexpr char * ON = "ON";
static const constexpr char * NOLEDS = "XXX XXX";

ErrorType BadgeTest::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   for(int i=0;i<ItemCount;++i) {
      Items[i].id = i;
	   Items[i].text = getRow(i);
   }
   sprintf(getRow(0),"Jump: %s", OFF);
   sprintf(getRow(1),"Fire: %s", OFF);
   sprintf(getRow(2),"Up: %s", OFF);
   sprintf(getRow(3),"Down: %s", OFF);
   //sprintf(getRow(4),"Right: %s", OFF);
   //sprintf(getRow(5),"Left: %s", OFF);
   //sprintf(getRow(6),"Lights: %s", NOLEDS);
   sprintf(getRow(7),"Version: %s", MyApp::get().getOTA().getCurrentApplicationVersion());
   sprintf(getRow(8),"Build Date: %s", MyApp::get().getOTA().getBuildDate());
//   sprintf(getRow(9),"Build Time: %s", MyApp::get().getOTA().getBuildTime());
   //sprintf(getRow(9),"BK:%s BL:%s R:%s",MyApp::get().getConfig().isPariedWithColor(BadgeColor::BLACK)?"Y":"N"
    //     , MyApp::get().getConfig().isPariedWithColor(BadgeColor::BLUE)?"Y":"N"
    //     , MyApp::get().getConfig().isPariedWithColor(BadgeColor::RED)?"Y":"N");
   //sprintf(getRow(10)," G:%s  P:%s W:%s",MyApp::get().getConfig().isPariedWithColor(BadgeColor::GREEN)?"Y":"N"
    //     , MyApp::get().getConfig().isPariedWithColor(BadgeColor::PURPLE)?"Y":"N"
     //    , MyApp::get().getConfig().isPariedWithColor(BadgeColor::WHITE)?"Y":"N");
   //MyApp::get().getGUI().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
	return ErrorType();
}

static uint32_t wasEscCount = 0;

libesp::BaseMenu::ReturnStateContext BadgeTest::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      switch(bme->getButton()) {
      case PIN_NUM_UP_BTN:
         if(bme->wasReleased()) sprintf(getRow(0),"Up: %s", OFF);
         else sprintf(getRow(0),"Up: %s", ON);
         //MyApp::get().setLEDs(MyApp::LEDS::LEFT_TWO);
         wasEscCount = 0;
         break;
      case PIN_NUM_DOWN_BTN:
         if(bme->wasReleased()) sprintf(getRow(1),"Down: %s", OFF);
         else sprintf(getRow(1),"Down: %s", ON);
         //MyApp::get().setLEDs(MyApp::LEDS::LEFT_THREE);
         wasEscCount = 0;
         break;
      case PIN_NUM_LEFT_BTN:
         if(bme->wasReleased()) sprintf(getRow(2),"Left: %s", OFF);
         else sprintf(getRow(2),"Left: %s", ON);
         //MyApp::get().setLEDs(MyApp::LEDS::RIGHT_ONE);
         wasEscCount = 0;
         break;
      case PIN_NUM_RIGHT_BTN:
         if(bme->wasReleased()) {
            sprintf(getRow(3),"Right: %s", OFF);
            wasEscCount = 0;
         } else {
            sprintf(getRow(3),"Right: %s", ON);
            ++wasEscCount;
         }
         break;
      default:
         break;
      }
	}


   if(2==wasEscCount) {
      nextState = MyApp::get().getMenuState();
   }

   //MyApp::get().getGUI().drawList(&this->MenuList);
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType BadgeTest::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
   wasEscCount = 0;
	return ErrorType();
}

