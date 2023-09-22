/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include "../appconfig.h"

class SimonSaysMenu: public AppBaseMenu {
   public:
      enum InternalState {
         INIT, RUN, SHUTDOWN
      };
public:
	SimonSaysMenu();
	virtual ~SimonSaysMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
   void showAll();
   void playSequence();
   void addColorToSequence();
private:
	QueueHandle_t QueueHandle;
   InternalState IState;
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
};

