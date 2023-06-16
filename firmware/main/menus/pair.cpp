/*
 *
 *      Author: cmdc0de
 */

#include "pair.h"
#include <device/display/display_device.h>
#include "error_type.h"
#include "esp_random.h"
#include "freertos.h"
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include <app/display_message_state.h>
#include <system.h>
#include "esp_http_client.h"
#include <cJSON.h>
#include "../vkeyboard.h"
#include <net/esp32inet.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::Button;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[PairMenu::QUEUE_SIZE*PairMenu::MSG_SIZE] = {0};
const char *PairMenu::LOGTAG = "PairMenu";
static VKeyboard VB;

PairMenu::PairMenu() : AppBaseMenu(), QueueHandle(),
	MenuList("Pair", Items, 0, 0, MyApp::get().getCanvasWidth(), MyApp::get().getCanvasHeight()
        , 0, ItemCount), PCode(), Initiate(false), Position(0), StartTimeWaitOtherBadge(0) {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

PairMenu::~PairMenu() {

}

enum ISTATE {
   INIT
   , SEND_INITIAL_PAIR
   , AWAITING_OTHER_BADGE
   , ENTER_PCODE
   , SEND_PAIR2
};

static ISTATE IState = INIT;

ErrorType PairMenu::onInit() {
   MenuList.selectTop();
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   IState = isInitator()?INIT:ENTER_PCODE;
   Position = 0;
   if(!isInitator()) {
      VB.init(VKeyboard::K1,7);
      sprintf(getRow(0),"Enter Pairing Code:");
   }
   for(int i=0;i<ItemCount;++i) {
      Items[i].id = i;
      Items[i].text = getRow(i);
      getRow(i)[0]='\0';
   }
   memset(&PCode[0],0,sizeof(PCode));
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
	return ErrorType();
}

static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

#ifdef LOCAL_WEB
static constexpr const char *Pair1_URL="http://192.168.5.41:5000/v1/cc/pair";
static constexpr const char *Pair2_URL="http://192.168.5.41:5000/v1/cc/status/%s";
static constexpr const char *Pair3_URL="http://192.168.5.41:5000/v1/cc/pair2/%s/%u";
#else
static constexpr const char *Pair1_URL="http://api.corncon.online:5000/v1/cc/pair";
static constexpr const char *Pair2_URL="http://api.corncon.online:5000/v1/cc/status/%s";
static constexpr const char *Pair3_URL="http://api.corncon.online:5000/v1/cc/pair2/%s/%u";
#endif

ErrorType PairMenu::pair1() {
   ErrorType et;
   char readBuf[2048] = {'\0'};
   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   config.url = Pair1_URL;
   config.timeout_ms = 15000;
   config.keep_alive_enable = true;
   config.skip_cert_common_name_check = true;
   config.method = HTTP_METHOD_POST;
   config.buffer_size_tx = 2048;
   config.buffer_size = 2048;


   esp_http_client_handle_t client = esp_http_client_init(&config);
   
   uint8_t macAddress[6];
   char macString[14];
   libesp::ESP32INet::get()->getSTAMacAddress(macAddress,macString);
   cJSON *root = cJSON_CreateObject();
   cJSON_AddStringToObject(root, "badge_name", MyApp::get().getConfig().getName());
   cJSON_AddStringToObject(root, "badge_id", &macString[0]);
   cJSON_AddNumberToObject(root, "rand", static_cast<double>(esp_random()));
   cJSON_AddNumberToObject(root, "badge_color", static_cast<double>(MyApp::get().getConfig().getMyBadgeColor()));
   const char *info = cJSON_Print(root);

   esp_http_client_set_header(client, "Content-Type", "application/json");
   et = esp_http_client_open(client, strlen(info));
   if (et.ok()) {
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
            int32_t bytes = esp_http_client_read(client,&readBuf[0],sizeof(readBuf));
            ESP_LOGI(LOGTAG,"status=%d contentLen=%d bytes =%d ret=%s",status,contentLen,bytes,&readBuf[0]);
            if(status!=201) {
               et = libesp::ErrorType::HTTP_SERVER_ERROR;
            } else {
               cJSON *rt = cJSON_Parse(&readBuf[0]);
               cJSON *pc = cJSON_GetObjectItem(rt,"pcode");
               strcpy(&PCode[0],cJSON_GetStringValue(pc));
               cJSON_Delete(rt);
            }
         }
      }
   }
   free((void *)info);
   cJSON_Delete(root);
   http_cleanup(client);
   return et;
}


libesp::ErrorType PairMenu::status(uint32_t timeNow) {
   ErrorType et;
   static uint32_t LastStatusTime = 0;

   if((timeNow-LastStatusTime)<=5000) {
      return ErrorType::TIMEOUT_ERROR;
   }

   LastStatusTime = timeNow;

   char readBuf[512] = {'\0'};
   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   char URL[128];
   sprintf(&URL[0],Pair2_URL,&PCode[0]);
   config.url = &URL[0];
   config.timeout_ms = 10000;
   config.keep_alive_enable = true;
   config.skip_cert_common_name_check = true;
   config.method = HTTP_METHOD_GET;
   config.buffer_size_tx = sizeof(readBuf);
   config.buffer_size = sizeof(readBuf);

   esp_http_client_handle_t client = esp_http_client_init(&config);

   esp_http_client_set_header(client, "Content-Type", "application/json");
   et = esp_http_client_open(client, 0);
   if (et.ok()) {
      int32_t content_length = esp_http_client_fetch_headers(client);
      int32_t status = esp_http_client_get_status_code(client);
      ESP_LOGI(LOGTAG,"status=%d contentLen=%d",status,content_length);
      //int32_t contentLen = esp_http_client_get_content_length(client);
      if(content_length>0 && status==200) {
         int32_t bytes = esp_http_client_read(client,&readBuf[0],sizeof(readBuf));
         ESP_LOGI(LOGTAG,"bytes=%d ret=%s",bytes,&readBuf[0]);
         cJSON *rt = cJSON_Parse(&readBuf[0]);
         cJSON *bid = cJSON_GetObjectItem(rt,"badge2_id");
         cJSON *bname = cJSON_GetObjectItem(rt,"badge2_name");
         cJSON *bcolor = cJSON_GetObjectItem(rt,"badge2_badge_color");
         et = MyApp::get().getConfig().setPairedColor(cJSON_GetStringValue(bid)
               , cJSON_GetStringValue(bname), &PCode[0], static_cast<BadgeColor>(cJSON_GetNumberValue(bcolor)));
         cJSON_Delete(rt);
      } else {
         et = libesp::ErrorType::TIMEOUT_ERROR;
      }
   }
   http_cleanup(client);
   return et;
}


ErrorType PairMenu::pair2() {
   ErrorType et;
   char readBuf[1024] = {'\0'};
   char URL[128];
   sprintf(&URL[0],Pair3_URL,&PCode[0],static_cast<uint32_t>(MyApp::get().getConfig().getMyBadgeColor()));

   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   config.url = &URL[0];
   config.timeout_ms = 15000;
   config.keep_alive_enable = true;
   config.skip_cert_common_name_check = true;
   config.method = HTTP_METHOD_POST;
   config.buffer_size_tx = sizeof(readBuf);
   config.buffer_size = sizeof(readBuf);


   esp_http_client_handle_t client = esp_http_client_init(&config);
   
   uint8_t macAddress[6];
   char macString[14];
   libesp::ESP32INet::get()->getSTAMacAddress(macAddress,macString);
   cJSON *root = cJSON_CreateObject();
   cJSON_AddStringToObject(root, "badge_name", MyApp::get().getConfig().getName());
   cJSON_AddStringToObject(root, "badge_id", &macString[0]);
   cJSON_AddNumberToObject(root, "rand", static_cast<double>(esp_random()));
   const char *info = cJSON_Print(root);

   esp_http_client_set_header(client, "Content-Type", "application/json");
   et = esp_http_client_open(client, strlen(info));
   if (et.ok()) {
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
            int32_t bytes = esp_http_client_read(client,&readBuf[0],sizeof(readBuf));
            ESP_LOGI(LOGTAG,"status=%d contentLen=%d bytes =%d ret=%s",status,contentLen,bytes,&readBuf[0]);
            if(status!=200) {
               et = libesp::ErrorType::HTTP_SERVER_ERROR;
            } else {
               cJSON *rt = cJSON_Parse(&readBuf[0]);
               cJSON *badge_id = cJSON_GetObjectItem(rt,   "initating_badge_id");
               cJSON *badge_name = cJSON_GetObjectItem(rt, "initating_badge_name");
               cJSON *badge_color = cJSON_GetObjectItem(rt,"initating_badge_color");
               et = MyApp::get().getConfig().setPairedColor(cJSON_GetStringValue(badge_id)
                     , cJSON_GetStringValue(badge_name), &PCode[0]
                     , static_cast<BadgeColor>(cJSON_GetNumberValue(badge_color)));
               cJSON_Delete(rt);
            }
         }
      }
   }
   free((void *)info);
   cJSON_Delete(root);
   http_cleanup(client);
   return et;
}

BaseMenu::ReturnStateContext PairMenu::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) { 
         switch(bme->getButton()) {
            case PIN_NUM_FIRE_BTN:
               if(IState==ENTER_PCODE) {
                  PCode[Position] = VB.getSelection();
                  if(++Position==sizeof(PCode)-1) {
                     PCode[Position] = '\0';
                  } 
               }
               break;
            case PIN_NUM_JUMP_BTN:
               if(ENTER_PCODE==IState) {
                  PCode[Position]={'\0'};
                  IState = SEND_PAIR2;
               } else {
                  nextState = MyApp::get().getMenuState();
               }
               break;
            case PIN_NUM_UP_BTN:
               if(ENTER_PCODE==IState) {
                  if(Position>=1) --Position;
                  PCode[Position]={'\0'};
               } else {
                  MenuList.moveUp();
               }
               break;
            case PIN_NUM_DOWN_BTN:
               if(ENTER_PCODE==IState) {
                  VB.reset();
               } else {
                  MenuList.moveDown();
               }
               break;
               /*
            case PIN_NUM_LEFT_BTN:
               if(ENTER_PCODE==IState) {
                  VB.moveLeft();
               }
               break;
            case PIN_NUM_RIGHT_BTN:
               if(ENTER_PCODE==IState) {
                  VB.moveRight();
               }
               break;
               */
            default:
               break;
         }
      }
      delete bme;
   }
   switch(IState) {
      case INIT:
         sprintf(getRow(0),"Initiating Pair...");
         IState = SEND_INITIAL_PAIR;
         break;
      case SEND_INITIAL_PAIR: 
         {
            //MyApp::get().setLEDs(MyApp::LEDS::LEFT_ONE);
	         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
            ErrorType et = this->pair1();
            if(et.ok()) {
               sprintf(getRow(1),"Single Use Pair Code: ");
               sprintf(getRow(2),"     %s", &PCode[0]);
               StartTimeWaitOtherBadge = libesp::FreeRTOS::getTimeSinceStart();
               IState = AWAITING_OTHER_BADGE;
            } else {
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                    , "Failed to initiate\nPairing", 2000);
            }
         }
         break;
      case AWAITING_OTHER_BADGE: 
         {
            //MyApp::get().setLEDs(MyApp::LEDS::LEFT_ONETWO);
            uint32_t now = libesp::FreeRTOS::getTimeSinceStart();
            ErrorType et = status(now);
            if(et.ok()) {
               //MyApp::get().setLEDs(MyApp::LEDS::LEFT_ONETWOTHREE);
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                    , "Pair Successful", 2000);
            } else if(et.getErrT()==ErrorType::TIMEOUT_ERROR) {
               if((now-StartTimeWaitOtherBadge)>MAX_WAIT_TIME_FOR_OTHER_BADGE) {
                  nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                    , "Timeout: waiting\nother badge", 2000);
               }
            } else {
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                    , "Pairing Failed", 2000);
            }
         }
         break;
      case ENTER_PCODE:
         {
            //MyApp::get().setLEDs(MyApp::LEDS::RIGHT_ONE);
            sprintf(getRow(1),"     %s", &PCode[0]);
         }
         break;
      case SEND_PAIR2:
         {
            //MyApp::get().setLEDs(MyApp::LEDS::RIGHT_ONETWO);
            ErrorType et = pair2();
            if(et.ok()) {
               //MyApp::get().setLEDs(MyApp::LEDS::RIGHT_ONETWOTHREE);
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                    , "Pair Successful", 2000);
            } else {
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                    , "Pairing failed\nEnsure you have\nthe corect PCode!", 2000);
            }
         }
         break;
   }
   MyApp::get().getGUI().drawList(&this->MenuList);
   if(IState==ENTER_PCODE) {
      VB.draw(MyApp::get().getDisplay(),50,100);
   }
	return ReturnStateContext(nextState);
}

ErrorType PairMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
   //MyApp::get().setLEDs(MyApp::LEDS::ALL_OFF);
	return ErrorType();
}


