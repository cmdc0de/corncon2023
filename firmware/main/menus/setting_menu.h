/*
 * setting_state.h
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include <device/display/layout.h>
#include "../appconfig.h"

class SettingMenu: public AppBaseMenu {
public:
  enum INTERNAL_STATE {
    SHOW_ALL = 0
    , AP_RUNNING
    , CALIBRATION
  };
public:
	SettingMenu();
	virtual ~SettingMenu();
   libesp::ErrorType doRegistration();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t QueueHandle;
   INTERNAL_STATE InternalState;
	libesp::DisplayGUIListData MenuList;
	libesp::DisplayGUIListItemData Items[4];
   char Name[AppConfig::MAX_NAME_LENGTH];
   uint8_t Position;
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
};


