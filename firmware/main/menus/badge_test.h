#pragma once

#include "appbase_menu.h"
#include <device/display/display_gui.h>

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
	libesp::DisplayGUIListData MenuList;
	libesp::DisplayGUIListItemData Items[9];
	QueueHandle_t InternalQueueHandler;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
  uint32_t ExitTimer;
};

