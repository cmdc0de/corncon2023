#pragma once

#include "appbase_menu.h"

namespace libesp {
	class DisplayDevice;
	class RGBColor;
}


class Pacman : public AppBaseMenu {
public:
	Pacman();
	virtual ~Pacman();
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t InternalQueueHandler;
   static constexpr const char *LOGTAG = "PACMAN";
};


