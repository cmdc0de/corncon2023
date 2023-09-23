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
         INIT
         , PLAY_SEQUENCE
         , TAKE_INPUT
         , WINNER
         , MESSAGE
         , SHUTDOWN
      };
public:
	SimonSaysMenu();
	virtual ~SimonSaysMenu();
   void setMultiplayer(bool b) {IsMultiplayer=b;}
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
   void showAll();
   void playSequence();
   void addColorToSequence();
   void drawWedge(uint8_t wedge);
private:
	QueueHandle_t QueueHandle;
   InternalState IState;
   uint16_t Position;
   bool IsMultiplayer;
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
};

