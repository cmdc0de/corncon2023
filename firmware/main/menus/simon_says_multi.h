/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include "../appconfig.h"
#include "error_type.h"
#include "etl/vector.h"

class SimonSaysMultiMenu: public AppBaseMenu {
   public:
      enum InternalState {
         INIT
         , START_SERVER
         , START_CLIENT
         , ACCEPT_PLAYERS
         , PLAY_SEQUENCE
         , TAKE_INPUT
         , WINNER
         , MESSAGE
         , SHUTDOWN
      };
   public:
      class ClientConnection {
      public:
         ClientConnection();
         virtual ~ClientConnection();
      public:
         libesp::ErrorType connect();
         libesp::ErrorType disconnect();
         libesp::ErrorType sendButtonPress(gpio_num_t button);
      private:
         int Connection;
      };
      class ServerContext {
      public:
         ServerContext();
         virtual ~ServerContext();
      public:
         libesp::ErrorType start();
         libesp::ErrorType stop();
         uint32_t getNumConnections() const;
         libesp::ErrorType getNewConnections();
         libesp::ErrorType readConnections();
      private:
         int ListenSocket;
         etl::vector<int,10> Connections;
      };
public:
	SimonSaysMultiMenu();
	virtual ~SimonSaysMultiMenu();
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
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
	static const char *LOGTAG;
};

