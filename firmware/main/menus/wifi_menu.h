#pragma once

#include "appbase_menu.h"
#include <net/wifi.h>
#include <net/networktimeprotocol.h>
#include <net/wifieventhandler.h>
#include "../vkeyboard.h"

class WiFiMenu: public AppBaseMenu, libesp::WiFiEventHandler {
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
   static const char *LOGTAG;
   static const char *MENUHEADER;
   static const char *TZKEY;
   static const uint32_t MAX_RETRY_CONNECT_COUNT = 5;

   static const uint32_t NOSTATE = 0;
   static const uint32_t CONNECTING = 1<<0;
   static const uint32_t CONNECTED = 1<<1;
   static const uint32_t WIFI_READY = 1<<2;
   static const uint32_t HAS_IP = 1<<3;
   static const uint32_t SCAN_COMPLETE = 1<<4;
   static const uint32_t AP_START = 1<<5;


   enum INTERNAL_STATE {
      INIT = 0
    , SCAN_RESULTS
    , DISPLAY_SINGLE_SSID
  };
public:
	WiFiMenu();
	virtual ~WiFiMenu();
   libesp::ErrorType connect();
   libesp::ErrorType disconnect();
   bool isConnected();
   libesp::ErrorType initWiFi();
   libesp::ErrorType clearConnectData();
   bool stopAP();
public:
   virtual libesp::ErrorType staStart();
   virtual libesp::ErrorType staStop();
   virtual libesp::ErrorType wifiReady();
	virtual libesp::ErrorType apStaConnected(wifi_event_ap_staconnected_t *info);
	virtual libesp::ErrorType apStaDisconnected(wifi_event_ap_stadisconnected_t *info);
	virtual libesp::ErrorType apStart();
	virtual libesp::ErrorType apStop();
	virtual libesp::ErrorType staConnected(system_event_sta_connected_t *info);
	virtual libesp::ErrorType staDisconnected(system_event_sta_disconnected_t *info);
	virtual libesp::ErrorType staGotIp(system_event_sta_got_ip_t *info);
	virtual libesp::ErrorType staScanDone(system_event_sta_scan_done_t *info);
	virtual libesp::ErrorType staAuthChange(system_event_sta_authmode_change_t *info);
   int32_t getServerID() const {return ServerID;}
protected:
   bool isFlagSet(uint32_t f) {return ((f&Flags)!=0);}
   libesp::ErrorType setWiFiConnectionData(const char *ssid, const char *pass);
   void setTZ();
   void displaySingleSid();
   BaseMenu* scan();
   BaseMenu* processScanList(ButtonManagerEvent *bme);
   BaseMenu* handleSingleSid(ButtonManagerEvent *bme);
   libesp::ErrorType onInit();
   ReturnStateContext onRun();
   libesp::ErrorType onShutdown();
private:
   libesp::WiFi MyWiFi;
   libesp::NTP NTPTime;
   uint32_t    Flags;
   uint16_t    ReTryCount;
   char        TimeZone[32];
   INTERNAL_STATE InternalState;
	libesp::DisplayGUIListItemData Items[12];
	libesp::DisplayGUIListData MenuList;
	QueueHandle_t InternalQueueHandler;
   VKeyboard VB;
   char WiFiPassword[libesp::WiFi::PASSWDTYPE::MAX_SIZE];
   uint16_t Position;
   int32_t ServerID;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
};

