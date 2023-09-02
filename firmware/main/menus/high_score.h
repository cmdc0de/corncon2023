/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include "../appconfig.h"

class HighScore: public AppBaseMenu {
public:
	HighScore();
	virtual ~HighScore();
   libesp::ErrorType submitScore(uint32_t score);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
   libesp::ErrorType fetchScores();
private:
	QueueHandle_t QueueHandle;
	libesp::DisplayGUIListData MenuList;
	libesp::DisplayGUIListItemData Items[12];
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
};

