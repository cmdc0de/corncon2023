/*
 *
 *      Author: cmdc0de
 */

#include "connection_details.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include "wifi_menu.h"
#include <app/display_message_state.h>
#include <system.h>
#include <net/esp32inet.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::Button;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[ConnectionDetails::QUEUE_SIZE*ConnectionDetails::MSG_SIZE] = {0};
const char *ConnectionDetails::LOGTAG = "ConnectionDetails";

ConnectionDetails::ConnectionDetails() : AppBaseMenu(), QueueHandle(),
	MenuList("Connection", Items, 35, 50, 155, 80, 0, ItemCount) {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

ConnectionDetails::~ConnectionDetails() {

}

void ConnectionDetails::updateMenu() {
   Items[0].id = 0;
   sprintf(getRow(0),"IsConnected?: %s", MyApp::get().getWiFiMenu()->isConnected()?"Yes":"No");
   Items[0].text = getRow(0);
   if(MyApp::get().getWiFiMenu()->isConnected()) {
      esp_netif_t * interface = libesp::ESP32INet::get()->getWifiSTAInterface();
      char buf[20];
      libesp::ESP32INet::get()->getIPv4(interface, &buf[0], sizeof(buf));
      Items[1].id = 1;
      sprintf(getRow(1),"IP Addr: %s", &buf[0]); 
	   Items[1].text = getRow(1);

	   Items[2].id = 2;
      libesp::ESP32INet::get()->getNetMask(interface, &buf[0], sizeof(buf));
      sprintf(getRow(2),"NetMask: %s", &buf[0]);
	   Items[2].text = getRow(2);

      Items[3].id = 3;
      libesp::ESP32INet::get()->getGateway(interface, &buf[0], sizeof(buf));
      sprintf(getRow(3),"Gateway %s", &buf[0]);
      Items[3].text = getRow(3);
     
      time_t now = 0;
      time(&now);
      struct tm timeinfo;
      memset(&timeinfo,0,sizeof(timeinfo));
      localtime_r(&now, &timeinfo);
      strftime(buf, sizeof(buf), "%c", &timeinfo);
      Items[4].id = 4;
      sprintf(getRow(4),"Time: %s", &buf[0]);
      Items[4].text = getRow(4);

      Items[5].id = 5;
      Items[5].text = "Disconnect";
   } else {
      Items[1].id = 1;
      Items[1].text = "Connect";
      Items[2].id = 2;
      Items[2].text = "";
      Items[3].id = 3;
      Items[3].text = "";
      Items[4].id = 4;
      Items[4].text = "";
      Items[5].id = 5;
      Items[5].text = "";
   }
}

ErrorType ConnectionDetails::onInit() {
   libesp::ESP32INet::get()->dumpToLog();
   MenuList.selectTop();
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   updateMenu();
   MyApp::get().getDisplay().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
	return ErrorType();
}


BaseMenu::ReturnStateContext ConnectionDetails::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) { 
         switch(bme->getButton()) {
            case PIN_NUM_SELECT_BTN:
               if(MyApp::get().getWiFiMenu()->isConnected()) {
                  if(MenuList.getSelectedItemID()==5) {
                     MyApp::get().getWiFiMenu()->disconnect();
                  }
               } else {
                  if(MenuList.getSelectedItemID()==1) {
                     ESP_LOGI(LOGTAG,"Connecting to WiFi");
                     MyApp::get().getWiFiMenu()->connect();
                  }
               }
               break;
            case PIN_NUM_BOT_BTN:
               nextState = MyApp::get().getMenuState();
               break;
            case PIN_NUM_TL_BTN:
               MenuList.moveUp();
               break;
            case PIN_NUM_BL_BTN:
               MenuList.moveDown();
               break;
            default:
               break;
         }
      }
      delete bme;
   }
   updateMenu();
   MyApp::get().getDisplay().drawList(&this->MenuList);
	return ReturnStateContext(nextState);
}

ErrorType ConnectionDetails::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


