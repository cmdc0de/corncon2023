#ifndef APP_BASE_MENU_H
#define APP_BASE_MENU_H

#include <app/basemenu.h>
#include <device/display/gui.h>
#include <device/hwbutton/buttonmanager.h>
#include <freertos.h>
#include "../pinconfig.h"

namespace libesp {
class GUIListData;
}

class AppBaseMenu : public libesp::BaseMenu {
public:
   typedef libesp::ButtonManager<10,2,6,2>::ButtonEvent ButtonManagerEvent;
	static const char *LOGTAG;
   static const uint32_t RowLength = 48;
	static const uint32_t NumRows = 12;
public:
	AppBaseMenu() : libesp::BaseMenu() {}
	virtual ~AppBaseMenu(){}
protected:
	static void clearListBuffer();
	static char *getRow(uint8_t row);
private:
	static char ListBuffer[NumRows][RowLength]; //height then width
};

#endif
