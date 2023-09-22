/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "simon_says.h"
#include <cstdint>
#include <device/display/display.h>
#include "device/display/color.h"
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
   RGBColor::RED
   , RGBColor::GREEN
   , RGBColor::BLUE
   , RGBColor(255,255,0)
   , RGBColor::WHITE
};

static etl::vector<uint8_t, 64> Sequence;

SimonSaysMenu::SimonSaysMenu() : AppBaseMenu(), QueueHandle(), IState(INIT) {
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SimonSaysMenu::~SimonSaysMenu() {

}

void SimonSaysMenu::addColorToSequence() {
   Sequence.push_back(rand()%NumColors);
}

void SimonSaysMenu::playSequence() {
   for(auto i=Sequence.begin(); i!=Sequence.end(); ++i) {
      MyApp::get().getDisplay().fillScreen(Colors[*i]);
      vTaskDelay(500/portTICK_PERIOD_MS);
      MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      vTaskDelay(500/portTICK_PERIOD_MS);
   }
}

void SimonSaysMenu::showAll() {
   const float StartingX = 120.0f;
   const float StartingY = 120.0f;
   float angleInc = static_cast<float>(2*3.14/NumColors);
   float angle = (3.14/180.0f)*60;
   float angle1Inc = (3.14/180.0f);
   float RadiusOfCircle = 120.0f;
   double px,py;
   for(int i=0;i<NumColors;++i) {
      for(float a=angle;a<angle+angleInc;a+=angle1Inc) {
         px = StartingX + RadiusOfCircle * std::cos(a);
         py = StartingY + RadiusOfCircle * std::sin(a);
         MyApp::get().getDisplay().drawLine(StartingX,StartingY,px,py,Colors[i]);
      }
      //px = StartingX + RadiusOfCircle * std::cos(angle);
      //py = StartingY + RadiusOfCircle * std::sin(angle);
      //MyApp::get().getDisplay().drawLine(StartingX,StartingY,px,py,Colors[i]);
      //ESP_LOGI(LOGTAG,"#%u: px: %0.2f  py: %0.2f\r\n", i, px, py);
      angle += angleInc;
   }
}


ErrorType SimonSaysMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
   IState = INIT;
   Sequence.clear();
	return ErrorType();
}


BaseMenu::ReturnStateContext SimonSaysMenu::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) {
	      nextState = MyApp::get().getMenuState();
      }
      delete bme;
   }
   switch(IState) {
      case INIT:
         showAll();
	      MyApp::get().getDisplay().drawString(40,100,"Simon Says",RGBColor::WHITE,RGBColor::BLACK,2,false);
         //playSequence();
         //addColorToSequence();
         IState = RUN;
         break;
      case RUN:
         break;
      case SHUTDOWN:
         break;
   }
   
	return ReturnStateContext(nextState);
}

ErrorType SimonSaysMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


