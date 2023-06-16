#include <stdlib.h>
#include <device/display/display_device.h>
#include "device/display/color.h"
#include "esp_netif_types.h"
#include "pacman.h"
#include "menu_state.h"
#include "../app.h"
#include "freertos.h"
#include "../art/sprits.h"
#include "../spaceinvader/sprits.h"

using libesp::RGBColor;
using libesp::FreeRTOS;
using libesp::ErrorType;
using libesp::BaseMenu;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[Pacman::QUEUE_SIZE*Pacman::MSG_SIZE] = {0};
static libesp::DCImage PacmanMap;
static libesp::DCImage PacmanSprite[2];

Pacman::Pacman() : AppBaseMenu() {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
   PacmanMap.bytes_per_pixel = 2;
   PacmanMap.height = 120;
   PacmanMap.width = 126;
   extern char pac_map_start[] asm("_binary_pacmanlevel1_map_start");
   extern char pac_map_end[]   asm("_binary_pacmanlevel1_map_end");
   ESP_LOGI(LOGTAG,"****size of pacmap %d ****************",(pac_map_end-pac_map_start));
   PacmanMap.pixel_data = &pac_map_start[0];

   PacmanSprite[0].height = getHeightinvader1(); //getHeightpacman1();
   PacmanSprite[0].width = getWidthinvader1();//getWidthpacman1();
   PacmanSprite[0].bytes_per_pixel = 2;
   PacmanSprite[0].pixel_data = reinterpret_cast<const char *>(getPixelDatainvader1());//(getPixelDatapacman1());
   PacmanSprite[1].height = getHeightinvader1a(); //getHeightpacman2();
   PacmanSprite[1].width = getWidthinvader1a(); //getWidthpacman2();
   PacmanSprite[1].bytes_per_pixel = 2;
   PacmanSprite[1].pixel_data =reinterpret_cast<const char *>(getPixelDatainvader1a());//(getPixelDatapacman2());
  // PacmanSprite[2].height = getHeightpacman2();
  // PacmanSprite[2].width = getWidthpacman2();
  // PacmanSprite[2].bytes_per_pixel = 2;
  // PacmanSprite[2].pixel_data = reinterpret_cast<const char *>(getPixelDatapacman2());
}

Pacman::~Pacman() {

}

enum INTERNAL_STATE {
	INIT, RENDER
};

static INTERNAL_STATE InternalState = INIT;

ErrorType Pacman::onInit() {
	InternalState = INIT;
   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   MyApp::get().getDisplay().drawImage(0,0,PacmanMap);
	return ErrorType();
}


static int32_t spriteIndex = 0;
static uint32_t LastTime = 0;
static const uint32_t ANIMATION_TIME = 250;

BaseMenu::ReturnStateContext Pacman::onRun() {
   BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      nextState = MyApp::get().getMenuState();
   } 
   //MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   //MyApp::get().getDisplay().drawImage(0,0,PacmanMap);
   MyApp::get().getDisplay().drawImage(135,50,PacmanSprite[spriteIndex]);


  uint32_t timeSinceLast = FreeRTOS::getTimeSinceStart()-LastTime;
   if(timeSinceLast>=ANIMATION_TIME) {
      LastTime = FreeRTOS::getTimeSinceStart();
      ++spriteIndex;
      if(spriteIndex>1) spriteIndex=0;
   } else {
      //ESP_LOGI(LOGTAG,"t");
   }
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType Pacman::onShutdown() {
	return ErrorType();
}

