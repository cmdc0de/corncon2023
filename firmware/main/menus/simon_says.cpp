/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "simon_says.h"
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

SimonSaysMenu::SimonSaysMenu() : AppBaseMenu(), QueueHandle() {
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SimonSaysMenu::~SimonSaysMenu() {

}


ErrorType SimonSaysMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
	return ErrorType();
}


BaseMenu::ReturnStateContext SimonSaysMenu::onRun() {
	BaseMenu *nextState = MyApp::get().getMenuState();
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      delete bme;
   }
   
	MyApp::get().getDisplay().drawString(40,100,"Simon Says",RGBColor::WHITE,RGBColor::BLACK,2,false);
	return ReturnStateContext(nextState);
}

ErrorType SimonSaysMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


