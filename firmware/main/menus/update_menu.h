/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include "../appconfig.h"

class UpdateMenu: public AppBaseMenu {
public:
	UpdateMenu();
	virtual ~UpdateMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t QueueHandle;
public:
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
};

