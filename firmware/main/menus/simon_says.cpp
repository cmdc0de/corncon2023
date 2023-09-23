/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "simon_says.h"
#include <cstdint>
#include <device/display/display.h>
#include "device/display/color.h"
#include "esp_random.h"
#include "freertos.h"
#include "hal/gpio_types.h"
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include <system.h>
#include <app/display_message_state.h>
#include "../app.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[SimonSaysMenu::QUEUE_SIZE*SimonSaysMenu::MSG_SIZE] = {0};
const char *SimonSaysMenu::LOGTAG = "SimonSays";

static int32_t NumColors = 5;
static RGBColor Colors[] = {
   RGBColor(0xFF,0xA5,0)
   , RGBColor::WHITE
   , RGBColor::BLUE
   , RGBColor(0x80,0,0x80)
   , RGBColor::RED
};

static gpio_num_t ButtonMap[] = {
   PIN_NUM_BOT_BTN
   , PIN_NUM_BL_BTN
   , PIN_NUM_TL_BTN
   , PIN_NUM_TR_BTN
   , PIN_NUM_BR_BTN
};


static const int32_t MAX_SEQUENCE = 32;
static etl::vector<uint8_t, MAX_SEQUENCE> Sequence;

SimonSaysMenu::SimonSaysMenu() : AppBaseMenu(), QueueHandle(), IState(INIT), Position(0), IsMultiplayer(false) {
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SimonSaysMenu::~SimonSaysMenu() {

}

void SimonSaysMenu::addColorToSequence() {
   for(int i=0;i<MAX_SEQUENCE;++i) {
      Sequence.push_back(esp_random()%NumColors);
   }
}
   
const float StartingX = 120.0f;
const float StartingY = 120.0f;

void SimonSaysMenu::drawWedge(uint8_t wedge) {
   static const float angleInc = static_cast<float>(2*3.14/NumColors);
   float radius = 120.0f;
   float drawAngle = (3.14/540.0f);
   float px,py;
   float angle = (3.14/180.0f)*55;
   float startAngle = angle+(wedge*angleInc);
   float endAngle = startAngle+angleInc;
   for(float a=startAngle;a<endAngle;a+=drawAngle) {
      px = StartingX + radius * std::cos(a);
      py = StartingY + radius * std::sin(a);
      MyApp::get().getDisplay().drawLine(StartingX,StartingY,px,py,Colors[wedge]);
   }
}

void SimonSaysMenu::playSequence() {
   uint16_t count = 0;
   int32_t delayTime = 500-(Position*10);
   for(auto i=Sequence.begin(); i!=Sequence.end() && count<=Position; ++i, ++count) {
      MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      MyApp::get().getDisplay().swap();
      vTaskDelay(delayTime/portTICK_PERIOD_MS);
      ESP_LOGI(LOGTAG,"#%u: %u  gpio: %d\r\n", count, *i, ButtonMap[*i]);
      drawWedge(*i);
      MyApp::get().getDisplay().swap();
      gpio_set_level(MyApp::get().getLEDForButton(ButtonMap[*i]),1);
      vTaskDelay(delayTime/portTICK_PERIOD_MS);
      gpio_set_level(MyApp::get().getLEDForButton(ButtonMap[*i]),0);
   }
}

void SimonSaysMenu::showAll() {
   float angleInc = static_cast<float>(2*3.14/NumColors);
   float angle = (3.14/180.0f)*75;
   float angle1Inc = (3.14/360.0f);
   float RadiusOfCircle = 120.0f;
   double px,py;
   for(int i=0;i<NumColors;++i) {
      for(float a=angle;a<angle+angleInc;a+=angle1Inc) {
         px = StartingX + RadiusOfCircle * std::cos(a);
         py = StartingY + RadiusOfCircle * std::sin(a);
         MyApp::get().getDisplay().drawLine(StartingX,StartingY,px,py,Colors[i]);
      }
      angle += angleInc;
   }
}

static uint64_t lastTime = libesp::FreeRTOS::getTimeSinceStart();

ErrorType SimonSaysMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
   IState = INIT;
   Sequence.clear();
   addColorToSequence();
   Position = 0;
   uint64_t lastTime = libesp::FreeRTOS::getTimeSinceStart();

	return ErrorType();
}

static int16_t InputButton = -1;

BaseMenu::ReturnStateContext SimonSaysMenu::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
   gpio_num_t button = GPIO_NUM_NC;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) {
         button = bme->getButton();
         gpio_set_level(MyApp::get().getLEDForButton(bme->getButton()),0);
      } else {
         gpio_set_level(MyApp::get().getLEDForButton(bme->getButton()),1);
      }
      delete bme;
   }
   if(IsMultiplayer) {
      MyApp::get().getDisplay().drawString(40,100,"Simon Says",RGBColor::WHITE,RGBColor::BLACK,2,false);
      MyApp::get().getDisplay().drawString(40,120,"Update to play",RGBColor::WHITE,RGBColor::BLACK,2,false);
      MyApp::get().getDisplay().drawString(40,140,"Muliplayer!!",RGBColor::WHITE,RGBColor::BLACK,2,false);
      if(button!=GPIO_NUM_NC) {
         nextState = MyApp::get().getMenuState();
      }
   } else {
      switch(IState) {
      case INIT:
         if(libesp::FreeRTOS::getTimeSinceStart()-lastTime > 1000) {
            MyApp::get().turnOffAllLEDs();
            if(Position >= NumColors) {
               Position = 0;
               lastTime = libesp::FreeRTOS::getTimeSinceStart();
               IState = MESSAGE;
            } else {
               lastTime = libesp::FreeRTOS::getTimeSinceStart();
               MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
               drawWedge(Position);
               gpio_set_level(MyApp::get().getLEDForButton(ButtonMap[Position]),1);
               ++Position;
            }
         }
         break;
      case PLAY_SEQUENCE:
         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
         playSequence();
         InputButton = -1;
         IState = TAKE_INPUT;
         break;
      case TAKE_INPUT:
         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
         if(button != GPIO_NUM_NC) {
            ++InputButton;
            if(ButtonMap[Sequence[InputButton]] == button) {
               if(InputButton == Position) {
                  if(Position >= MAX_SEQUENCE) {
                     IState = WINNER;
                  } else {
                     IState = MESSAGE;
                     lastTime = libesp::FreeRTOS::getTimeSinceStart();
                     ++Position;
                  }
               }
            } else {
               IState = SHUTDOWN;
            }
         }
         break;
      case MESSAGE:
         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
         char buf[32];
         sprintf(buf,"Start Round: %d",Position+1);
         MyApp::get().getDisplay().drawString(40,100,buf,RGBColor::WHITE,RGBColor::BLACK,2,false);
         if(libesp::FreeRTOS::getTimeSinceStart()-lastTime > 1500) {
            lastTime = libesp::FreeRTOS::getTimeSinceStart();
            IState = PLAY_SEQUENCE;
         }
         break;
      case WINNER:
         nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), "WINNER!!!", 5000);
         break;
      case SHUTDOWN:
         nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), "Try Again", 3000);
         break;
      }
   }
	return ReturnStateContext(nextState);
}

ErrorType SimonSaysMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


