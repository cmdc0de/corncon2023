
#include "appconfig.h"
#include "error_type.h"
#include "timezones.h"
#include <nvs_memory.h>
#include <string.h>
#include <esp_partition.h>

using libesp::NVS;
using libesp::ErrorType;

AppConfig::AppConfig(libesp::NVS *s) : Storage(s), Name(), SleepTime(2), Flags(0), TimeZone() {
   LedEnabled= 1;
   memset(&Name[0],0,sizeof(Name));
   memset(&TimeZone[0],0,sizeof(TimeZone));
   strcpy(&TimeZone[0],"UTC");
}


AppConfig::~AppConfig() {

}

void espsettz(const char *niceTZ) {
   const char *esptz = getESPTZ(niceTZ);
   setenv("TZ", esptz, 1);
   tzset();
   ESP_LOGI("espsettz","Timezone set to %s: %s", niceTZ, esptz);
}

libesp::ErrorType AppConfig::init() {
   ErrorType et;
   {
      uint32_t len = static_cast<uint32_t>(sizeof(Name));
      et = Storage->getValue(NAME_KEY,&Name[0],len);
      if(!et.ok()) {
         ESP_LOGI(LOGTAG,"Failed to load name %s", et.toString());
      }
      et = Storage->getValue(SLEEP_KEY,SleepTime);
      if(!et.ok()) {
         ESP_LOGI(LOGTAG,"Failed to load sleep time %s", et.toString());
      }
      et = Storage->getValue(FLAGS_KEY,Flags);
      if(!et.ok()) {
         ESP_LOGI(LOGTAG,"Failed to load light enable %s", et.toString());
      }
      len = sizeof(TimeZone);
      et = Storage->getValue(TZ_KEY,&TimeZone[0],len);
      if(!et.ok()) {
         ESP_LOGI(LOGTAG,"Failed to load TZ %s", et.toString());
      }
      espsettz(&TimeZone[0]);
   }
   return et;
}
   
bool AppConfig::isRegistered() {
   uint16_t r = 0;
   if(Storage->getValue(REGISTRATION_KEY,r).ok()) {
      if(r==1) return true;
   }
   return false;
}

libesp::ErrorType AppConfig::setRegistered(bool b) {
   ErrorType et;
   uint16_t r = b?1:0;
   et = Storage->setValue(REGISTRATION_KEY,r);
   return et;
}

libesp::ErrorType AppConfig::setName(const char *name) {
   ErrorType et;
   strncpy(&Name[0],name,sizeof(Name));
   et = Storage->setValue(NAME_KEY,name);
   if(et.ok()) {
      et = setRegistered(false);
   }
   return et;
}

libesp::ErrorType AppConfig::setTZ(const char *tz) {
   ErrorType et;
   strncpy(&TimeZone[0],tz,sizeof(TimeZone));
   et = Storage->setValue(TZ_KEY,tz);
   espsettz(&TimeZone[0]);
   return et;
}

libesp::ErrorType AppConfig::setSleepMin(uint16_t s) {
   ErrorType et;
   SleepTime = s;
   et = Storage->setValue(SLEEP_KEY,s);
   return et;
}

libesp::ErrorType AppConfig::setLedsEnable(bool b) {
   ErrorType et;
   LedEnabled = b?1:0;
   et = Storage->setValue(FLAGS_KEY,Flags);
   return et;
}


ErrorType AppConfig::hasWiFiBeenSetup() {
  char data[64] = {"\0"};
  uint32_t len = sizeof(data);
  ErrorType et = Storage->getValue(WIFISID, &data[0],len);
  Sid = &data[0];
  ESP_LOGI(LOGTAG,"SID = %s",Sid.c_str());
   if(et.ok()) {
      len = sizeof(data);
      et = Storage->getValue(WIFIPASSWD, &data[0],len);
      if(et.ok()) {
         WifiPassword = &data[0];
         //ESP_LOGI(LOGTAG,"P: %s",WifiPassword.c_str());
      } else {
         ESP_LOGI(LOGTAG,"failed to load password: %d %s", et.getErrT(), et.toString()); 
      }
  } else {
    ESP_LOGI(LOGTAG,"failed to load wifisid: %d %s", et.getErrT(), et.toString()); 
  }
  return et;
}

ErrorType AppConfig::clearConnectData() {
  ErrorType et = Storage->eraseKey(WIFISID);
  if(!et.ok()) {
    ESP_LOGI(LOGTAG,"failed to erase key ssid: %d %s", et.getErrT(), et.toString()); 
  } 
  et = Storage->eraseKey(WIFIPASSWD);
  if(!et.ok()) {
    ESP_LOGI(LOGTAG,"failed to erase key password: %d %s", et.getErrT(), et.toString()); 
  }
  return et;
}


ErrorType AppConfig::setWifiData(const char *sid, const char *password) {
   ErrorType et = Storage->setValue(WIFISID,sid);
   if(et.ok()) {
      Sid = sid;
      ESP_LOGI(LOGTAG,"Saving SID = %s", Sid.c_str());
      et = Storage->setValue(WIFIPASSWD,password);
      if(et.ok()) {
         WifiPassword = password;
         ESP_LOGI(LOGTAG,"Saving passwrd: %s",WifiPassword.c_str());
      }
   } else {
      ESP_LOGI(LOGTAG,"failed to save ssid: %d %s", et.getErrT(), et.toString());
   }
   return et;
}
