/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include "../appconfig.h"

class PairMenu: public AppBaseMenu {
public:
	PairMenu();
	virtual ~PairMenu();
   void initatePair(bool b) {Initiate = b;}
   bool isInitator() {return Initiate;}
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
   libesp::ErrorType pair1();
   libesp::ErrorType status(uint32_t timeNow);
   libesp::ErrorType pair2();
private:
	QueueHandle_t QueueHandle;
	libesp::DisplayGUIListData MenuList;
	libesp::DisplayGUIListItemData Items[6];
   char PCode[28];
   bool Initiate;
   uint8_t Position;
   uint32_t StartTimeWaitOtherBadge;
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
   static constexpr const uint32_t MAX_WAIT_TIME_FOR_OTHER_BADGE = 60000;
};

