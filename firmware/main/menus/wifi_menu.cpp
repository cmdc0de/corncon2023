#include "wifi_menu.h"
#include "../app.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include "app/basemenu.h"
#include "menu_state.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[WiFiMenu::QUEUE_SIZE*WiFiMenu::MSG_SIZE] = {0};
const char *WiFiMenu::LOGTAG = "WIFIMENU";
const char *WiFiMenu::MENUHEADER = "SSID              RSSI  CH";


void time_sync_cb(struct timeval *tv) {
    ESP_LOGI(WiFiMenu::LOGTAG, "Notification of a time synchronization event");
}

WiFiMenu::WiFiMenu() : AppBaseMenu(), MyWiFi(), NTPTime(), Flags(0), ReTryCount(0), TimeZone()
   , InternalState(INIT), Items()
  , MenuList(MENUHEADER, Items, 0, 0, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), 0, ItemCount), InternalQueueHandler(), VB(), WiFiPassword(), Position(0) {

	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

ErrorType WiFiMenu::initWiFi() {
  MyWiFi.setWifiEventHandler(this);
  ErrorType et = MyWiFi.init(WIFI_MODE_APSTA);
  if(et.ok()) {
    et = NTPTime.init(MyApp::get().getNVS(),true,time_sync_cb);
  }
  return et;
}

ErrorType WiFiMenu::disconnect() {
   ErrorType et = MyWiFi.disconnect();
   if(et.ok()) {
      Flags = (Flags&~(CONNECTING|CONNECTED));
   }
   return et;
}

ErrorType WiFiMenu::connect() {
  ErrorType et = MyWiFi.connect(MyApp::get().getConfig().getWiFiSid()
        ,MyApp::get().getConfig().getWiFiPassword(),WIFI_AUTH_OPEN);
  if(et.ok()) {
    Flags|=CONNECTING;
  }
  return et;
}

bool WiFiMenu::isConnected() {
  return (Flags&CONNECTED)!=0;
}


WiFiMenu::~WiFiMenu() {

}

static etl::vector<libesp::WiFiAPRecord,16> ScanResults;

ErrorType WiFiMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
   clearListBuffer();
   for(int i=0;i<ItemCount;++i) {
      Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
   }
 
   ScanResults.clear();
   InternalState = INIT;
   Position = 0;
   memset(&WiFiPassword[0],0,sizeof(WiFiPassword));
	return ErrorType();
}

BaseMenu* WiFiMenu::scan() {
   BaseMenu *nextState = this;
   ESP_LOGI(LOGTAG,"*****************************Scanresutls empty()");
   clearListBuffer();
   ScanResults.empty();
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   MenuList.selectedItem=0;//remove selected item
   ErrorType et = MyWiFi.scan(ScanResults,false);
   if(et.ok()) {
      //ESP_LOGI(LOGTAG,"in vector: %u", ScanResults.size());
      for(uint32_t i = 0;i<ScanResults.size() && i< NumRows;++i) {
         snprintf(getRow(i),AppBaseMenu::RowLength,"%-15.14s  %4d  %3d"
         , ScanResults[i].getSSID().c_str(), ScanResults[i].getRSSI(), ScanResults[i].getPrimary());
      }
      //MyApp::get().getGUI().drawList(&MenuList);
      InternalState = SCAN_RESULTS;
   } else {
      nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                     , "WifiScan Failed (try again)", 2000);
   }
   return nextState;
}

void WiFiMenu::displaySingleSid() {
   uint32_t selectedItem = MenuList.selectedItem;
   ESP_LOGI(LOGTAG,"selectItem %u", selectedItem);
   InternalState = DISPLAY_SINGLE_SSID;
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   char rowBuffer[80];
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"SSID: %-22.21s", ScanResults[selectedItem].getSSID().c_str());
   MyApp::get().getDisplay().drawString(5,10,&rowBuffer[0],RGBColor::WHITE);
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"Auth: %s",  ScanResults[selectedItem].getAuthModeString());
   MyApp::get().getDisplay().drawString(5,20,&rowBuffer[0],RGBColor::WHITE);
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"RSSI: %d", static_cast<int32_t>(ScanResults[selectedItem].getRSSI()));
   MyApp::get().getDisplay().drawString(5,30,&rowBuffer[0],RGBColor::WHITE);
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"Channel: %d", static_cast<int32_t>(ScanResults[selectedItem].getPrimary()));
   MyApp::get().getDisplay().drawString(5,40,&rowBuffer[0],RGBColor::WHITE);
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"Caps: B:%s N:%s G:%s LR:%s"
      ,ScanResults[selectedItem].isWirelessB()?"Y":"N", ScanResults[selectedItem].isWirelessN()?"Y":"N"
      ,ScanResults[selectedItem].isWirelessG()?"Y":"N", ScanResults[selectedItem].isWirelessLR()?"Y":"N");
   MyApp::get().getDisplay().drawString(5,50,&rowBuffer[0],RGBColor::WHITE);
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"Pwd: %s", &WiFiPassword[0]);
   MyApp::get().getDisplay().drawString(5,60,&rowBuffer[0],RGBColor::WHITE);
   VB.init(VKeyboard::K1,7);
   Position = 0;
   memset(&WiFiPassword[0],0,sizeof(WiFiPassword));
}

BaseMenu* WiFiMenu::processScanList(ButtonManagerEvent *bme) {
   BaseMenu *nextState = this;
   if(nullptr==bme) return nextState;
   if(bme->wasReleased())  {
      switch(bme->getButton()) {
         case PIN_NUM_UP_BTN:
            MenuList.moveUp();
            //MyApp::get().getGUI().drawList(&MenuList);
         break;
         case PIN_NUM_DOWN_BTN:
            MenuList.moveDown();
            //MyApp::get().getGUI().drawList(&MenuList);
         break;
         case PIN_NUM_FIRE_BTN:
            displaySingleSid();
            //VB.draw(MyApp::get().getDisplay(),50, 80);
         break;
         case PIN_NUM_JUMP_BTN:
            nextState = MyApp::get().getMenuState();
         default:
         break;
      }
   }
   return nextState;
}

libesp::BaseMenu * WiFiMenu::handleSingleSid(ButtonManagerEvent *bme) {
   libesp::BaseMenu *nextState = this;
   if(bme==nullptr) return nextState;
   if(bme->wasReleased()) {
      switch(bme->getButton()) {
         case PIN_NUM_UP_BTN:
         case PIN_NUM_DOWN_BTN:
            VB.reset();
            break;
         case PIN_NUM_LEFT_BTN:
            VB.moveLeft();
            break;
         case PIN_NUM_RIGHT_BTN:
            VB.moveRight();
            break;
            /*
         case PIN_NUM_JUMP_BTN:
            if(WiFiPassword[0]!='\0') {
               WiFiPassword[Position] = '\0';
               ErrorType et = MyApp::get().getConfig().setWifiData(ScanResults[MenuList.selectedItem].getSSID().c_str(), &WiFiPassword[0]);
               if(!et.ok()) nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                     , "WiFiConfig Saved failed", 2000);
               else nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                     , "WiFiConfig Saved Successfully", 2000);
            }
            break;
         case PIN_NUM_FIRE_BTN:
            WiFiPassword[Position] = VB.getSelection();
            if(++Position==AppConfig::PASSWDTYPE::MAX_SIZE-1) {
               WiFiPassword[Position] = '\0';
               ErrorType et = MyApp::get().getConfig().setWifiData(ScanResults[MenuList.selectedItem].getSSID().c_str(), &WiFiPassword[0]);
               if(!et.ok()) ESP_LOGE(LOGTAG,"Error saving name %s",et.toString());
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                     , "Max Name Length reached, wifi config saved ", 2000);
            }
            break;
            */
         default:
            break;
      }
   }
   //VB.draw(MyApp::get().getDisplay(),50, 80);
   char rowBuffer[80];
   snprintf(&rowBuffer[0],sizeof(rowBuffer),"Pwd: %s", &WiFiPassword[0]);
   MyApp::get().getDisplay().drawString(5,60,&rowBuffer[0],RGBColor::WHITE);
   return nextState;
}


libesp::BaseMenu::ReturnStateContext WiFiMenu::onRun() {
	BaseMenu *nextState = this;
   //ESP_LOGI(LOGTAG, "CURRENT STATE = %u, size of ScanResults: %u", InternalState, ScanResults.size());
   ButtonManagerEvent *bme = nullptr;
   if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      switch(InternalState) {
         case INIT:
         //   nextState = scan();
         break;
         case SCAN_RESULTS:
            nextState = processScanList(bme);
         break;
         case DISPLAY_SINGLE_SSID:
            nextState = handleSingleSid(bme);
         break;
      }
      delete bme;
   } else if(InternalState==INIT) {
      nextState = scan();
   }

	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType WiFiMenu::onShutdown() {
   MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
	return ErrorType();
}

// wifi handler
ErrorType WiFiMenu::staStart() {
  ErrorType et;
  //ESP_LOGI(LOGTAG, __PRETTY_FUNCTION__ );
  return et;
}

ErrorType WiFiMenu::staStop() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::wifiReady() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=WIFI_READY;
  return et;
}

ErrorType WiFiMenu::apStaConnected(wifi_event_ap_staconnected_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::apStaDisconnected(wifi_event_ap_stadisconnected_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::apStart() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::apStop() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::staConnected(system_event_sta_connected_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=CONNECTED;
  Flags=(Flags&~CONNECTING);
  return et;
}

ErrorType WiFiMenu::staDisconnected(system_event_sta_disconnected_t *info) {
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags=(Flags&~(CONNECTED|HAS_IP));
  NTPTime.stop();
  if(++ReTryCount<MAX_RETRY_CONNECT_COUNT && !MyApp::get().isSleeping()) {
    return connect();
  }
  return ErrorType(ErrorType::MAX_RETRIES);
}

ErrorType WiFiMenu::staGotIp(system_event_sta_got_ip_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=HAS_IP;
  ReTryCount = 0;
  NTPTime.start();
  return et;
}

ErrorType WiFiMenu::staScanDone(system_event_sta_scan_done_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=SCAN_COMPLETE;
  return et;
}

ErrorType WiFiMenu::staAuthChange(system_event_sta_authmode_change_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

