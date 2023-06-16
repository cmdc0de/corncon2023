/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include "../appconfig.h"

class ConnectionDetails: public AppBaseMenu {
public:
	ConnectionDetails();
	virtual ~ConnectionDetails();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
   void updateMenu();
private:
	QueueHandle_t QueueHandle;
	libesp::GUIListData MenuList;
	libesp::GUIListItemData Items[6];
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
};

