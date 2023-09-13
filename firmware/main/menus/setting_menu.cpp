/*
 * setting_state.cpp
 *
 *      Author: cmdc0de
 */

#include "setting_menu.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
//#include "wifi_menu.h"
#include <app/display_message_state.h>
#include "game_of_life.h"
#include "menu3d.h"
#include <system.h>
#include "../vkeyboard.h"
#include "../timezones.h"
#include "update_menu.h"
#include "esp_http_client.h"
#include <cJSON.h>
#include <net/esp32inet.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::Button;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[SettingMenu::QUEUE_SIZE*SettingMenu::MSG_SIZE] = {0};
const char *SettingMenu::LOGTAG = "SettingMenu";
VKeyboard VB;

SettingMenu::SettingMenu() : AppBaseMenu(), QueueHandle() 
	, InternalState(INTERNAL_STATE::SHOW_ALL), MenuList("Setting (bot saves)", Items, 35, 40, 120, 100, 0, ItemCount), Name(), Position(0) {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SettingMenu::~SettingMenu() {

}

enum STATES {
  INIT
  , ENTER_NAME
  , ENTER_NUMBER
//  , ENTER_BOOL
//  , ENTER_TZ
};

static STATES State = INIT;

ErrorType SettingMenu::onInit() {
   memset(&Name[0],0,sizeof(Name));
   if(MyApp::get().getConfig().isNameSet()) {
      strcpy(&Name[0],MyApp::get().getConfig().getName());
   }
	ButtonManagerEvent* *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(QueueHandle, &pe, 0)) {
			delete pe;
		}
	}
   Items[0].id = 0;
   sprintf(getRow(0),"Name: %s", &Name[0]);
   Items[0].text = getRow(0);
	Items[1].id = 1;
   sprintf(getRow(1),"Set Sleep Time: %d", MyApp::get().getConfig().getSleepMin());
	Items[1].text = getRow(1);

   Items[2].id = 2;
   sprintf(getRow(2),"Check for updates");
   Items[2].text = getRow(2);
   
   Items[3].id = 3;
   sprintf(getRow(3),"Clear WIFI Config");
   Items[3].text = getRow(3);

	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   MyApp::get().getDisplay().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
   State = INIT;
	return ErrorType();
}

BaseMenu::ReturnStateContext SettingMenu::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) {
         switch(bme->getButton()) {
            case PIN_NUM_SELECT_BTN:
               {
                  switch(State) {
                  case INIT:
                     Position = 0;
                     memset(&Name[0],0,sizeof(Name));
                     switch(MenuList.getSelectedItemID()) {
                        case 0:
                           State = ENTER_NAME;
                           VB.init(VKeyboard::K1,7);
                           break;
                        case 1:
                           State = ENTER_NUMBER;
                           VB.init(VKeyboard::N1,5);
                           break;
                        case 2:
                           nextState = MyApp::get().getUpdateMenu();
                           break;
                        case 3:
                           nextState = MyApp::get().getDisplayMessageState(this,"Clearing wifi data", 2000);
                           MyApp::get().getConfig().clearConnectData();
                           break;
                     }
                     break;
                  case ENTER_NAME:
                     Name[Position] = VB.getSelection();
                     if(++Position==AppConfig::MAX_NAME_LENGTH-1) {
                        Name[Position] = '\0';
                        ErrorType et = MyApp::get().getConfig().setName(&Name[0]);
                        if(!et.ok()) ESP_LOGE(LOGTAG,"Error saving name %s",et.toString());
                        nextState = MyApp::get().getDisplayMessageState(this, "Max Name Length reached", 2000);
                     } 
                     sprintf(getRow(0),"Name: %s", &Name[0]);
                     break;
                  case ENTER_NUMBER: 
                     {
                     int32_t v = VB.getSelection() - '0';
                     sprintf(getRow(1),"Set Sleep Time: %d", v);
                     break;
                     }
                  }
               }
            break;
            case PIN_NUM_BL_BTN: 
            {
               switch(State) {
                  case INIT:
                     MenuList.moveDown();
                     break;
                  case ENTER_NAME:
                     if(Position>=1) --Position;
                     Name[Position]='\0';
                     sprintf(getRow(0),"Name: %s", &Name[0]);
                     break;
                  default:
                     VB.reset();
                     break;
               }
            }
            break;
            case PIN_NUM_TL_BTN:
            {
               switch(State) {
                  case INIT:
                     MenuList.moveUp();
                     break;
                  default:
                     VB.moveLeft();
                     break;
               }
            }
            break;
            case PIN_NUM_TR_BTN:
            {
               switch(State) {
                  case INIT:
                     break;
                  default:
                     VB.moveRight();
                     break;
               }
               break;
            }
            case PIN_NUM_BOT_BTN:
            {
               switch(State) {
               case INIT:
                  nextState = MyApp::get().getMenuState();
                  break;
               case ENTER_NAME: 
                  {
                        ErrorType et = MyApp::get().getConfig().setName(&Name[0]);
                        if(!et.ok()) {
                           ESP_LOGE(LOGTAG,"Error saving name %s",et.toString());
                           nextState = MyApp::get().getDisplayMessageState(this, "Failed to save name", 2000);
                        } else {
                           nextState = MyApp::get().getDisplayMessageState(this, "Name Saved Successfully", 2000);
                        }
                  }
                  break;
               case ENTER_NUMBER: 
                  {
                        ErrorType et = MyApp::get().getConfig().setSleepMin(uint16_t(VB.getSelection()-'0'));
                        if(!et.ok()) {
                           ESP_LOGE(LOGTAG,"Error saving sleep time %s",et.toString());
                           nextState = MyApp::get().getDisplayMessageState(this, "Failed to save sleep time", 2000);
                        } else {
                           nextState = MyApp::get().getDisplayMessageState(this, "Sleep Time Saved\nSuccessfully", 2000);
                        }

                  }
                  break;
               default:
                  break;
               }
               break;
            }
            default:
            break;
         }
      }
      delete bme;
   }
   MyApp::get().getDisplay().drawList(&this->MenuList);
   if(State!=INIT) { 
      VB.draw(MyApp::get().getDisplay(),50, 110);
   }

	return ReturnStateContext(nextState);
}

ErrorType SettingMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


