/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "update_menu.h"
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
static uint8_t QueueBuffer[UpdateMenu::QUEUE_SIZE*UpdateMenu::MSG_SIZE] = {0};
const char *UpdateMenu::LOGTAG = "UpdateMenu";

class UpdateProgress : public libesp::OTAProgress {
public:
   UpdateProgress() : libesp::OTAProgress() {}
protected:
   virtual void onUpdate(const PROGRESS &p) {
      char buffer[64] = {'\0'};
      char buffer1[64] = {'\0'};
      char buffer2[64] = {'\0'};
	   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      uint32_t delayTime = 1000;
      switch(p) {
         case INIT:
            sprintf(&buffer[0], "INIT");
            break;
         case CONNECTED:
            sprintf(&buffer[0], "CONNECTED");
            break;
         case HTTP_READ:
            sprintf(&buffer[0], "HTTP_READ");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            delayTime = 200;
            break;
         case HTTP_READ_ERROR:
            sprintf(&buffer[0], "HTTP_READ_ERROR");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case NO_NEW_VERSION_AVAIL:
            sprintf(&buffer[0], "NO_NEW_VERSION_AVAIL");
            sprintf(&buffer1[0], "Current: %s", this->getCurrentVersion());
            sprintf(&buffer2[0], "Downloaded: %s", this->getDownloadVersion());
            break;
         case IMAGE_HEADER_CHECK_SUCCESSFUL:
            sprintf(&buffer[0], "IMAGE_HEADER_CHECK_SUCCESSFUL");
            sprintf(&buffer1[0], "Current: %s", this->getCurrentVersion());
            sprintf(&buffer2[0], "Downloaded: %s", this->getDownloadVersion());
            break;
         case OTA_BEGIN_FAILED:
            sprintf(&buffer[0], "OTA_BEGIN_FAILED");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case OTA_WRITE_UPDATE_START:
            sprintf(&buffer[0], "OTA_WRITE_UPDATE_START");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case OTA_FAILED_WRITE:
            sprintf(&buffer[0], "OTA_FAILED_WRITE");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case COMPLETE:
            sprintf(&buffer[0], "COMPLETE");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case PARITION_SWAP_COMPLETE:
            sprintf(&buffer[0], "PARITION_SWAP_COMPLETE");
            sprintf(&buffer1[0], "Current: %s", this->getCurrentVersion());
            sprintf(&buffer2[0], "Downloaded: %s", this->getDownloadVersion());
            break;
      }
      MyApp::get().getDisplay().drawString(45,60,&buffer[0]);
      MyApp::get().getDisplay().drawString(45,80,&buffer1[0]);
      MyApp::get().getDisplay().drawString(45,100,&buffer2[0]);
      MyApp::get().getDisplay().swap();
	   vTaskDelay(delayTime / portTICK_RATE_MS);
   }
};



UpdateMenu::UpdateMenu() : AppBaseMenu(), QueueHandle() {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

UpdateMenu::~UpdateMenu() {

}


ErrorType UpdateMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
	return ErrorType();
}


BaseMenu::ReturnStateContext UpdateMenu::onRun() {
	BaseMenu *nextState = MyApp::get().getMenuState();
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      delete bme;
   }

   UpdateProgress UP;
   ErrorType et = MyApp::get().getOTA().run(&UP);
   if(!et.ok()) {
      char buf[64];
      sprintf(&buf[0],"Error: %s", et.toString());
	   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      MyApp::get().getDisplay().drawString(45,120,&buf[0]);
      MyApp::get().getDisplay().swap();
	   vTaskDelay(5000 / portTICK_RATE_MS);
   }

	return ReturnStateContext(nextState);
}

ErrorType UpdateMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


