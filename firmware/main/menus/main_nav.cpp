#include <stdlib.h>
#include <device/display/display_device.h>
#include "device/display/color.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "main_nav.h"
#include "math/point.h"
#include "menu_state.h"
#include "../app.h"
#include "freertos.h"


using libesp::RGBColor;
using libesp::FreeRTOS;
using libesp::ErrorType;
using libesp::BaseMenu;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MainNav::QUEUE_SIZE*MainNav::MSG_SIZE] = {0};
static libesp::DCImage MainMap;

MainNav::MainNav() : AppBaseMenu(), InternalQueueHandler(0), AvatarPos(0,0) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
   MainMap.bytes_per_pixel = 2;
   MainMap.height = 128;
   MainMap.width = 110;
   MainMap.pixel_data = 0;
}

MainNav::~MainNav() {

}


ErrorType MainNav::onInit() {
   AvatarPos.setX(54);
   AvatarPos.setY(114);
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
	return ErrorType();
}

BaseMenu::ReturnStateContext MainNav::onRun() {
	BaseMenu *nextState = this;
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   MyApp::get().getDisplay().drawImage(0,0,MainMap);
   MyApp::get().getDisplay().drawString(AvatarPos.getX(), AvatarPos.getY(),"X");
   ButtonManagerEvent *bme = nullptr;
   //bool wasFireBtnReleased = false;
   uint16_t *pMap = reinterpret_cast<uint16_t*>(0);
   libesp::Point2Ds newPos(AvatarPos);
   int32_t arrayPos = -1;
   
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      if(!bme->wasReleased()) {
         switch(bme->getButton()) {
            case PIN_NUM_UP_BTN:
               newPos.setY(AvatarPos.getY()-1);
               if(newPos.getY()<116 && newPos.getY()>5) arrayPos = -2;
               break;
            case PIN_NUM_DOWN_BTN:
               newPos.setY(AvatarPos.getY()+1);
               if(newPos.getY()<116 && newPos.getY()>5) arrayPos = -2;
               break;
            case PIN_NUM_FIRE_BTN:
               break;
               /*
            case PIN_NUM_LEFT_BTN:
               newPos.setX(AvatarPos.getX()-1);
               if(newPos.getX()>4 && newPos.getX()<105) arrayPos = -2;
               break;
            case PIN_NUM_RIGHT_BTN:
               newPos.setX(AvatarPos.getX()+1);
               if(newPos.getX()>4 && newPos.getX()<105) arrayPos = -2;
               break;
               */
            default:
               break;
         }
         if(-1!=arrayPos) {
            arrayPos = (newPos.getY()*MainMap.width) + newPos.getX();
            ESP_LOGI(LOGTAG,"%x",pMap[arrayPos]);
            if(0==pMap[arrayPos]) {
               AvatarPos = newPos;
            } else {
               ESP_LOGI(LOGTAG,"CAN'T");
            }
         }
      }
   }
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MainNav::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
	return ErrorType();
}

