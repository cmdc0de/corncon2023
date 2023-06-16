#pragma once

#include "appbase_menu.h"
#include <device/display/layout.h>

class BadgeTest: public AppBaseMenu {
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
   static const char *LOGTAG;
	BadgeTest();
	virtual ~BadgeTest();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData MenuList;
	libesp::GUIListItemData Items[11];
	QueueHandle_t InternalQueueHandler;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
};

