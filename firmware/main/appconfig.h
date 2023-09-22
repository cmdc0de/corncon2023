#pragma once

#include <error_type.h>
#include <net/wifi.h>

namespace libesp {
   class NVS;
}

class AppConfig {
public:
   static constexpr const uint32_t MAX_NAME_LENGTH = 14;
   static constexpr const char *NAME_KEY="BADGE_NAME";
   static constexpr const char *SLEEP_KEY="SLEEP_KEY";
   static constexpr const char *FLAGS_KEY="FLAGS_KEY";
   static constexpr const char *TZ_KEY="TZ_KEY";
   static constexpr const char *REGISTRATION_KEY="REG_KEY";
   static constexpr const char *LOGTAG = "AppConfig";
   static constexpr const char *WIFISID = "WIFISID";
   static constexpr const char *WIFIPASSWD="WIFIPASSWD";
   typedef libesp::WiFi::SSIDTYPE SSIDTYPE;
   typedef libesp::WiFi::PASSWDTYPE PASSWDTYPE;
public:
   AppConfig(libesp::NVS *s);
   ~AppConfig();
public:
   libesp::ErrorType init();
public:
   bool isNameSet() {return Name[0]!='\0';}
   const char *getName() {return &Name[0];}
   uint16_t getSleepMin() {return SleepTime;}
   bool ledsEnabled() {return LedEnabled;}
   libesp::ErrorType hasWiFiBeenSetup();
   libesp::ErrorType clearConnectData();
   const char *getWiFiSid() {return Sid.c_str();}
   const char *getWiFiPassword() {return WifiPassword.c_str();}
   const char *getTZ() { return &TimeZone[0];}
   bool isRegistered();
   uint16_t getPairCount() const;
public:
   libesp::ErrorType setName(const char *name);
   libesp::ErrorType setSleepMin(uint16_t s);
   libesp::ErrorType setLedsEnable(bool b);
   libesp::ErrorType setWifiData(const char *sid, const char *password);
   libesp::ErrorType setTZ(const char *tz);
   libesp::ErrorType setRegistered(bool b);
private:
   libesp::NVS *Storage;
   char Name[MAX_NAME_LENGTH];
   uint16_t SleepTime;
   union {
      struct {
         uint32_t LedEnabled:1;
      };
      uint32_t Flags;
   };
   SSIDTYPE Sid;
   PASSWDTYPE WifiPassword;
   char TimeZone[32];
};
