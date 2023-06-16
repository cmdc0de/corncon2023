#pragma once

#include "appbase_menu.h"
#include <math/point.h>

namespace libesp {
	class DisplayDevice;
	class RGBColor;
}


class MainNav : public AppBaseMenu {
public:
	MainNav();
	virtual ~MainNav();
   static constexpr const char *LOGTAG = "MainNav";
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t InternalQueueHandler;
   libesp::Point2Ds AvatarPos;
};


