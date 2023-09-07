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
	, InternalState(INTERNAL_STATE::SHOW_ALL), MenuList("Setting (jump saves)", Items, 0, 0, MyApp::get().getCanvasWidth()
   , MyApp::get().getCanvasHeight(), 0, ItemCount), Name(), Position(0) {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SettingMenu::~SettingMenu() {

}

enum STATES {
  INIT
  , ENTER_NAME
  , ENTER_NUMBER
  , ENTER_BOOL
  , ENTER_TZ
};

static STATES State = INIT;
static bool BValue = true;
static uint32_t TZPos = 19;

ErrorType SettingMenu::onInit() {
   memset(&Name[0],0,sizeof(Name));
   if(MyApp::get().getConfig().isNameSet()) {
      strcpy(&Name[0],MyApp::get().getConfig().getName());
   }
   BValue = MyApp::get().getConfig().ledsEnabled();
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
   sprintf(getRow(2),"LEDs Enabled: %s", (const char *)(MyApp::get().getConfig().ledsEnabled()?"No":"Yes"));
	Items[2].text = getRow(2);

   Items[3].id = 3;
   sprintf(getRow(3),"Registered: %s", MyApp::get().getConfig().isRegistered()?"YES":"NO");
   Items[3].text = getRow(3);

   Items[4].id = 4;
   sprintf(getRow(4),"TZ: %s", MyApp::get().getConfig().getTZ());
   Items[4].text = getRow(4);

   Items[5].id = 5;
   sprintf(getRow(5),"Check for updates");
   Items[5].text = getRow(5);
   
   Items[6].id = 6;
   sprintf(getRow(6),"Color: %s", MyApp::get().getConfig().getMyBadgeColorStr());
   Items[6].text = getRow(6);
   
   Items[7].id = 7;
   sprintf(getRow(7),"Clear WIFI Config");
   Items[7].text = getRow(7);

	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   MyApp::get().getDisplay().drawList(&this->MenuList);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
   State = INIT;
	return ErrorType();
}

static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

#ifdef LOCAL_WEB
static constexpr const char *REG_URL="http://192.168.5.41:5000/v1/cc/reg";
#else
static constexpr const char *REG_URL="http://api.corncon.online:5000/v1/cc/reg";
#endif

ErrorType SettingMenu::doRegistration() {
   ErrorType et;
   char readBuf[1024] = {'\0'};
   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   config.url = REG_URL;
   config.timeout_ms = 25000;
   config.keep_alive_enable = true;
   config.skip_cert_common_name_check = true;
   config.method = HTTP_METHOD_POST;
   config.buffer_size_tx = 1024;
   config.buffer_size = 1024;

   esp_http_client_handle_t client = esp_http_client_init(&config);
   
   uint8_t macAddress[6];
   char macString[14];
   libesp::ESP32INet::get()->getSTAMacAddress(macAddress,macString);
   cJSON *root = cJSON_CreateObject();
   cJSON_AddStringToObject(root, "badge_name", MyApp::get().getConfig().getName());
   cJSON_AddStringToObject(root, "badge_id", &macString[0]);
   const char *info = cJSON_Print(root);
  
   esp_http_client_set_header(client, "Content-Type", "application/json");
   et = esp_http_client_open(client, strlen(info));
   if (et.ok()) {
      //esp_http_client_set_post_field(client, info, strlen(info));
      ESP_LOGI(LOGTAG,"Posting: %s",info);
      int wlen = esp_http_client_write(client, info, strlen(info));
      if (wlen < 0) {
         ESP_LOGE(LOGTAG, "Write failed");
         et = libesp::ErrorType::HTTP_SERVER_ERROR;
      } else {
         int32_t content_length = esp_http_client_fetch_headers(client);
         if(content_length>0) {
            int32_t status = esp_http_client_get_status_code(client);
            int32_t contentLen = esp_http_client_get_content_length(client);
    		   //vTaskDelay(1000 / portTICK_RATE_MS);
            int32_t bytes = esp_http_client_read(client,&readBuf[0],sizeof(readBuf));
            ESP_LOGI(LOGTAG,"status=%d contentLen=%d bytes =%d ret=%s",status,contentLen,bytes,&readBuf[0]);
            if(status!=200) {
               et = libesp::ErrorType::HTTP_SERVER_ERROR;
            } else {
               et = MyApp::get().getConfig().setRegistered(true);
               if(!et.ok()) ESP_LOGI(LOGTAG,"setting reg failed: %s", et.toString());
               else ESP_LOGI(LOGTAG,"Saved registration flag");
            }
         }
      }
   }
   free((void *)info);
   cJSON_Delete(root);
   http_cleanup(client);
   return et;
}

BaseMenu::ReturnStateContext SettingMenu::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) {
         switch(bme->getButton()) {
            case PIN_NUM_FIRE_BTN:
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
                           State = ENTER_BOOL;
                           break;
                        case 3:
                           if(!MyApp::get().getConfig().isRegistered()) {
                              if(doRegistration().ok()) {
                                 nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                                       , "Registration successful", 2000);
                              } else {
                                 nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                                       , "Registration failed\nTry later", 2000);
                              }
                           } else {
                              nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                                 , "Already Registered", 2000);
                           }
                           break;
                        case 4:
                           State = ENTER_TZ;
                           break;
                        case 5:
                           nextState = MyApp::get().getUpdateMenu();
                           break;
                        case 6:
                           break;
                        case 7:
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
                  case ENTER_BOOL:
                     BValue = !BValue;
                     sprintf(getRow(2),"Disable LEDs: %s", (const char *)(BValue?"No":"Yes"));
                     break;
                  case ENTER_TZ:
                     break;
                  }
               }
            break;
            case PIN_NUM_UP_BTN: 
            {
               switch(State) {
                  case INIT:
                     MenuList.moveUp();
                     break;
                  case ENTER_TZ:
                     if(TZPos!=0) --TZPos;
                     sprintf(getRow(4),"TZ: %s", getTZ(TZPos));
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
            case PIN_NUM_DOWN_BTN:
            {
               switch(State) {
                  case INIT:
                     MenuList.moveDown();
                     break;
                  case ENTER_TZ:
                     ++TZPos;
                     if(TZPos>getTotalTimeZones()) TZPos = 0;
                     sprintf(getRow(4),"TZ: %s", getTZ(TZPos));
                     break;
                  default:
                     VB.reset();
                     break;
               }
            }
            break;
            /*
            case PIN_NUM_LEFT_BTN:
            {
               switch(State) {
                  case INIT:
                     MenuList.selectTop();
                     break;
                  default:
                     VB.moveLeft();
                     break;
               }
            }
            break;
            case PIN_NUM_RIGHT_BTN:
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
            */
            case PIN_NUM_RIGHT_BTN:
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
               case ENTER_BOOL:
                  {
                        ErrorType et = MyApp::get().getConfig().setLedsEnable(BValue);
                        if(!et.ok()) {
                           ESP_LOGE(LOGTAG,"Error saving led enable %s",et.toString());
                           nextState = MyApp::get().getDisplayMessageState(this, "Failed to save\nLED Enable", 2000);
                        } else {
                           nextState = MyApp::get().getDisplayMessageState(this, "LED Enable Saved\nSuccessfully", 2000);
                        }

                  }
                  break;
               case ENTER_TZ:
                  {
                        ErrorType et = MyApp::get().getConfig().setTZ(getTZ(TZPos));
                        if(!et.ok()) {
                           ESP_LOGE(LOGTAG,"Error saving TZ %s",et.toString());
                           nextState = MyApp::get().getDisplayMessageState(this, "Failed to save TZ", 2000);
                        } else {
                           nextState = MyApp::get().getDisplayMessageState(this, "TZ Saved Successfully", 2000);
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
   if(State!=INIT && State!=ENTER_BOOL && State!=ENTER_TZ) {
      VB.draw(MyApp::get().getDisplay(),50, 100);
   }

	return ReturnStateContext(nextState);
}

ErrorType SettingMenu::onShutdown() {
	//MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


