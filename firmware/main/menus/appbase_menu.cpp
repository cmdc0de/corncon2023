
#include "appbase_menu.h"
#include "device/touch/XPT2046.h"
#include "math/point.h"
#include "../app.h"
//#include "calibration_menu.h"
#include "gui_list_processor.h"
#include <esp_log.h>

using libesp::TouchNotification;
using libesp::Point2Ds;

const char *AppBaseMenu::LOGTAG = "AppBaseMenu";

char AppBaseMenu::ListBuffer[AppBaseMenu::NumRows][AppBaseMenu::RowLength] = {0};

void AppBaseMenu::clearListBuffer() {
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
}

char *AppBaseMenu::getRow(uint8_t row) {
	if(row>=NumRows) return nullptr;
	return &ListBuffer[row][0];
}


