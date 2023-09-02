
#include "appbase_menu.h"
#include "../app.h"
#include <esp_log.h>

const char *AppBaseMenu::LOGTAG = "AppBaseMenu";

char AppBaseMenu::ListBuffer[AppBaseMenu::NumRows][AppBaseMenu::RowLength] = {0};

void AppBaseMenu::clearListBuffer() {
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
}

char *AppBaseMenu::getRow(uint8_t row) {
	if(row>=NumRows) return nullptr;
	return &ListBuffer[row][0];
}


