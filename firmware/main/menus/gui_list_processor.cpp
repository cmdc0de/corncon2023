/*
 * gui_list_processor.cpp
 *
 *      Author: cmdc0de
 */

#include "gui_list_processor.h"
#include <device/display/gui.h>
#include "../app.h"
#include <device/display/display_device.h>
#include <esp_log.h>

using namespace libesp;
static const char *LOGTAG = "GUIListProcessor";

int32_t GUIListProcessor::process(libesp::GUIListData *pl, uint16_t itemC) {
	return 1;
}


