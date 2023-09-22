/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "sleep_menu.h"
#include <device/display/display_device.h>
#include "device/display/color.h"
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include <system.h>
#include <net/ota.h>
#include <app/display_message_state.h>
#include "../app.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[SleepMenu::QUEUE_SIZE*SleepMenu::MSG_SIZE] = {0};
const char *SleepMenu::LOGTAG = "SleepMenu";

SleepMenu::SleepMenu() : AppBaseMenu(), QueueHandle() {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SleepMenu::~SleepMenu() {

}


ErrorType SleepMenu::onInit() {
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   MyApp::get().getDisplay().drawString(40,100,"Sleeping",RGBColor::WHITE,RGBColor::BLACK,2,false);
   MyApp::get().goToSleep();
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
	return ErrorType();
}


BaseMenu::ReturnStateContext SleepMenu::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 50)) {
      ESP_LOGI(LOGTAG,"HERE");
      if(bme->wasReleased()) {
	      nextState = MyApp::get().getMenuState();
      }
      delete bme;
   }

	return ReturnStateContext(nextState);
}

ErrorType SleepMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
   MyApp::get().wakeUp();
	return ErrorType();
}


