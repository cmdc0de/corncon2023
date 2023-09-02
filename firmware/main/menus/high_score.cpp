/*
 *
 *      Author: cmdc0de
 */

#include "high_score.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include <app/display_message_state.h>
#include <system.h>
#include "esp_http_client.h"
#include <cJSON.h>
#include <net/esp32inet.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::Button;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[HighScore::QUEUE_SIZE*HighScore::MSG_SIZE] = {0};
const char *HighScore::LOGTAG = "HighScore";
static constexpr const char *MENUHEADER = "#       Name         Score";

HighScore::HighScore() : AppBaseMenu(), QueueHandle(),
	MenuList(MENUHEADER, Items, 0, 0, MyApp::get().getCanvasWidth(), MyApp::get().getCanvasHeight()
        , 0, ItemCount) {
	
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

HighScore::~HighScore() {

}

enum ISTATE {
   INIT
   , GET_SCORES
   , SHOW_SCORES
};

static ISTATE IState;

ErrorType HighScore::onInit() {
   MenuList.selectTop();
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   IState = INIT;
   for(int i=0;i<ItemCount;++i) {
      Items[i].id = i;
      Items[i].text = getRow(i);
      getRow(i)[0]='\0';
   }
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
	return ErrorType();
}

static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

#ifdef LOCAL_WEB
static constexpr const char *HIGHSCORE_URL="http://192.168.5.41:5000/v1/cc/scores";
static constexpr const char *HIGHSCORE_URL_POST="http://192.168.5.41:5000/v1/cc/score";
#else
static constexpr const char *HIGHSCORE_URL="http://api.corncon.online:5000/v1/cc/scores";
static constexpr const char *HIGHSCORE_URL_POST="http://api.corncon.online:5000/v1/cc/score";
#endif

ErrorType HighScore::submitScore(uint32_t score) {
   ErrorType et;
   char readBuf[2048] = {'\0'};
   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   config.url = HIGHSCORE_URL_POST;
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
   cJSON_AddNumberToObject(root, "score", score);
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
            } 
         }
      }
   }
   free((void *)info);
   cJSON_Delete(root);
   http_cleanup(client);
   return et;
}

ErrorType HighScore::fetchScores() {
   ErrorType et;
   char readBuf[2048] = {'\0'};
   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   config.url = HIGHSCORE_URL;
   config.timeout_ms = 15000;
   config.keep_alive_enable = true;
   config.skip_cert_common_name_check = true;
   config.method = HTTP_METHOD_GET;
   config.buffer_size_tx = 2048;
   config.buffer_size = 2048;

   esp_http_client_handle_t client = esp_http_client_init(&config);
   
   esp_http_client_set_header(client, "Content-Type", "application/json");
   et = esp_http_client_open(client, 0);
   if (et.ok()) {
      int32_t content_length = esp_http_client_fetch_headers(client);
      if(content_length>0) {
         int32_t status = esp_http_client_get_status_code(client);
         int32_t contentLen = esp_http_client_get_content_length(client);
         int32_t bytes = esp_http_client_read(client,&readBuf[0],sizeof(readBuf));
         ESP_LOGI(LOGTAG,"status=%d contentLen=%d bytes =%d ret=%s",status,contentLen,bytes,&readBuf[0]);
         if(status!=200) {
            et = libesp::ErrorType::HTTP_SERVER_ERROR;
         } else {
            cJSON *root = cJSON_Parse(&readBuf[0]);
            if(cJSON_IsArray(root)) {
               int32_t arraySize = cJSON_GetArraySize(root);
               for(int i=0;i<arraySize && i<ItemCount;++i) {
                  cJSON *item = cJSON_GetArrayItem(root,i);
                  cJSON *name = cJSON_GetObjectItem(item,"badge_name");
                  cJSON *score = cJSON_GetObjectItem(item,"score");
                  sprintf(getRow(i),"%2d: %-14.13s  %6d", i+1
                        ,cJSON_GetStringValue(name)
                        ,static_cast<int32_t>(cJSON_GetNumberValue(score)));
               }
            }
            cJSON_Delete(root);
         }
      }
   }
   http_cleanup(client);
   return et;
}

BaseMenu::ReturnStateContext HighScore::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) { 
         switch(bme->getButton()) {
            case PIN_NUM_FIRE_BTN:
               break;
            case PIN_NUM_LEFT_BTN:
               nextState = MyApp::get().getMenuState();
               break;
            case PIN_NUM_UP_BTN:
               MenuList.moveUp();
               break;
            case PIN_NUM_DOWN_BTN:
               MenuList.moveDown();
               break;
            default:
               break;
         }
      }
      delete bme;
   }
   switch(IState) {
      case INIT:
         MyApp::get().getDisplay().drawString(10,20,"Fetching Scores");
         IState = GET_SCORES;
         break;
      case GET_SCORES: 
         {
            ErrorType et = fetchScores();
            if(et.ok()) {
               IState = SHOW_SCORES;
            } else {
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                     , "Failed to fetch scores", 2000);
            }
         }
         break;
      case SHOW_SCORES:
         MyApp::get().getDisplay().drawList(&this->MenuList);
         break;
   }
	return ReturnStateContext(nextState);
}

ErrorType HighScore::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


