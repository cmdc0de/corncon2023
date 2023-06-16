
#include "appconfig.h"
#include "error_type.h"
#include "timezones.h"
#include <nvs_memory.h>
#include <string.h>
#include <esp_partition.h>

using libesp::NVS;
using libesp::ErrorType;

int32_t getBadgeColor() {
   const esp_partition_t *partition = esp_partition_find_first((esp_partition_type_t)0xFD, ESP_PARTITION_SUBTYPE_ANY, "storage");
   if(partition) {
      int32_t read_data = -1;
      esp_partition_read(partition, 0, &read_data, sizeof(read_data));
      ESP_LOGI("badge COLOR","********************: %d",read_data);
      return read_data;
   } else {
      ESP_LOGE("FAIL","FAILED TO OPEN PARITION");
   }
   return -1;
}

AppConfig::AppConfig(libesp::NVS *s) :
   Storage(s), Name(), SleepTime(2), Flags(0), TimeZone(), PairedBadgeColors()
   , MyBadgeColor(BadgeColor::WHITE) {

   LedEnabled= 1;
   memset(&Name[0],0,sizeof(Name));
   memset(&TimeZone[0],0,sizeof(TimeZone));
   strcpy(&TimeZone[0],"UTC");
   for(int i=0;i<BadgeColor::TOTAL_COLORS;++i) {
      PairedBadgeColors[i] = false;
   }
}


AppConfig::~AppConfig() {

}

void espsettz(const char *niceTZ) {
   const char *esptz = getESPTZ(niceTZ);
   setenv("TZ", esptz, 1);
   tzset();
   ESP_LOGI("espsettz","Timezone set to %s: %s", niceTZ, esptz);
}

const char *BadgeColorStr[BadgeColor::TOTAL_COLORS] = {
   "BLACK"
   , "RED"
   , "WHITE"
   , "BLUE"
   , "PURPLE"
   , "GREEN"
};

const char *AppConfig::getMyBadgeColorStr() const {
   return BadgeColorStr[MyBadgeColor];
}

const char *AppConfig::getBadgeColorStr(const BadgeColor &bc) const {
   if(bc<BadgeColor::TOTAL_COLORS) {
      return BadgeColorStr[bc];
   }
   return getMyBadgeColorStr();
}

libesp::ErrorType AppConfig::init() {
   ErrorType et;
   MyBadgeColor = (BadgeColor)getBadgeColor();
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
      len = static_cast<uint32_t>(sizeof(PairedBadgeColors));
      et = Storage->getBlob(PAIRED_BADGES, &PairedBadgeColors[0], len);
      if(!et.ok()) {
         ESP_LOGI(LOGTAG,"Failed to load Paired %s", et.toString());
      }
      PairedBadgeColors[MyBadgeColor] = true;
      //dump Paried bades
      for(int i=0;i<BadgeColor::TOTAL_COLORS;++i) {
         ESP_LOGI(LOGTAG,"Badge Color: %s. Paired wtih: %s",getBadgeColorStr(BadgeColor(i)),
               PairedBadgeColors[i]?"YES":"NO");
      }
   }
   return et;
}
   
libesp::ErrorType AppConfig::setPairedColor(const char *bid, const char * bname, const char *pcode
      , const BadgeColor &bc) {
   ErrorType et;
   ESP_LOGI(LOGTAG,"bid = %s, bname = %s, pcode = %s, color= %s", bid, bname, pcode, getBadgeColorStr(bc));
   if(bc<TOTAL_COLORS) {
      PairedBadgeColors[bc] = true;
      et = Storage->setBlob(PAIRED_BADGES, &PairedBadgeColors[0], sizeof(PairedBadgeColors) );
   }
   return et;
}

bool AppConfig::isPariedWithColor(const BadgeColor &bc) {
   if(bc<TOTAL_COLORS) {
      return PairedBadgeColors[bc];
   }
   return false;
}

uint16_t AppConfig::getPairCount() const {
   uint16_t c = 0;
   for(int i=0;i<TOTAL_COLORS;++i) {
      if(PairedBadgeColors[i]) ++c;
   }
   return c;
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
   }
   return et;
}
