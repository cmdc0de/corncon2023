/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "simon_says_multi.h"
#include "error_type.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <cstdint>
#include <device/display/display.h>
#include "device/display/color.h"
#include "esp_random.h"
#include "freertos.h"
#include "hal/gpio_types.h"
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include <system.h>
#include <app/display_message_state.h>
#include "../app.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

static StaticQueue_t ButtonQueue;
static uint8_t QueueBuffer[SimonSaysMultiMenu::QUEUE_SIZE*SimonSaysMultiMenu::MSG_SIZE] = {0};
const char *SimonSaysMultiMenu::LOGTAG = "SimonSaysMulti";

static int32_t NumColors = 5;
static RGBColor Colors[] = {
   RGBColor(0xFF,0xA5,0)
   , RGBColor::WHITE
   , RGBColor::BLUE
   , RGBColor(0x80,0,0x80)
   , RGBColor::RED
};

static gpio_num_t ButtonMap[] = {
   PIN_NUM_BOT_BTN
   , PIN_NUM_BL_BTN
   , PIN_NUM_TL_BTN
   , PIN_NUM_TR_BTN
   , PIN_NUM_BR_BTN
};

SimonSaysMultiMenu::ServerContext::ServerContext() : ListenSocket(-1), Connections() {

}

SimonSaysMultiMenu::ServerContext::~ServerContext() {
   stop();
}

libesp::ErrorType SimonSaysMultiMenu::ServerContext::start() {
   ErrorType et;
   if(ListenSocket<0) {
      struct sockaddr_in serv_addr;
      ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
      if (ListenSocket < 0) {
         ESP_LOGE(LOGTAG,"ERROR opening socket");
         return errno;
      }
      int opt = 1;
      setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      int flags = fcntl(ListenSocket, F_GETFL);
      if (fcntl(ListenSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
         ESP_LOGE(LOGTAG, "Unable to set socket non blocking");
      }
      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons(5000);
      if (bind(ListenSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
         ESP_LOGE(LOGTAG,"ERROR on binding");
         return errno;
      }
      listen(ListenSocket,5);
   } else {
      ESP_LOGE(LOGTAG,"Already started");
   }
   return et;
}

libesp::ErrorType SimonSaysMultiMenu::ServerContext::stop() {
   ErrorType et;
   if(ListenSocket>=0) {
      close(ListenSocket);
      ListenSocket = -1;
   }
   for(auto i=Connections.begin();i!=Connections.end();++i) {
      close(*i);
   }
   Connections.clear();
   return et;
}

uint32_t SimonSaysMultiMenu::ServerContext::getNumConnections() const {
   return Connections.size();
}

libesp::ErrorType SimonSaysMultiMenu::ServerContext::getNewConnections() {
   ErrorType et;
   struct sockaddr_in cli_addr;
   socklen_t clilen = sizeof(cli_addr);
   int newsockfd = accept(ListenSocket, (struct sockaddr *) &cli_addr, &clilen);
   if (newsockfd < 0) {
      if(errno!=EWOULDBLOCK) {
         ESP_LOGE(LOGTAG,"ERROR on accept");
      }
      return errno;
   }
   int flags = fcntl(newsockfd, F_GETFL);
   if (fcntl(newsockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
      ESP_LOGE(LOGTAG, "Unable to set socket non blocking");
   }
   Connections.push_back(newsockfd);
   return et;
}

libesp::ErrorType SimonSaysMultiMenu::ServerContext::readConnections() {
   ErrorType et;
   for(auto i=Connections.begin();i!=Connections.end();++i) {
      char buffer[256];
      int n = read(*i,buffer,255);
      if(n<0) {
         if(errno!=EWOULDBLOCK) {
            ESP_LOGE(LOGTAG,"ERROR on read");
         }
      } else if(n==0) {
         close(*i);
         Connections.erase(i);
      } else {
         buffer[n] = 0;
         //ESP_LOGI(LOGTAG,"Received %d bytes: %s",n,buffer);
         //processBuffer(buffer,n);
      }
   } 
   return et;
}

static const int32_t MAX_SEQUENCE = 32;
static etl::vector<uint8_t, MAX_SEQUENCE> Sequence;

SimonSaysMultiMenu::SimonSaysMultiMenu() : AppBaseMenu(), QueueHandle(), IState(INIT), Position(0) {
	QueueHandle = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&QueueBuffer[0],&ButtonQueue);
}

SimonSaysMultiMenu::~SimonSaysMultiMenu() {

}

void SimonSaysMultiMenu::addColorToSequence() {
   for(int i=0;i<MAX_SEQUENCE;++i) {
      Sequence.push_back(esp_random()%NumColors);
   }
}
   
const float StartingX = 120.0f;
const float StartingY = 120.0f;

void SimonSaysMultiMenu::drawWedge(uint8_t wedge) {
   static const float angleInc = static_cast<float>(2*3.14/NumColors);
   float radius = 120.0f;
   float drawAngle = (3.14/540.0f);
   float px,py;
   float angle = (3.14/180.0f)*55;
   float startAngle = angle+(wedge*angleInc);
   float endAngle = startAngle+angleInc;
   for(float a=startAngle;a<endAngle;a+=drawAngle) {
      px = StartingX + radius * std::cos(a);
      py = StartingY + radius * std::sin(a);
      MyApp::get().getDisplay().drawLine(StartingX,StartingY,px,py,Colors[wedge]);
   }
}

void SimonSaysMultiMenu::playSequence() {
   uint16_t count = 0;
   int32_t delayTime = 500-(Position*10);
   for(auto i=Sequence.begin(); i!=Sequence.end() && count<=Position; ++i, ++count) {
      MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      MyApp::get().getDisplay().swap();
      vTaskDelay(delayTime/portTICK_PERIOD_MS);
      ESP_LOGI(LOGTAG,"#%u: %u  gpio: %d\r\n", count, *i, ButtonMap[*i]);
      drawWedge(*i);
      MyApp::get().getDisplay().swap();
      gpio_set_level(MyApp::get().getLEDForButton(ButtonMap[*i]),1);
      vTaskDelay(delayTime/portTICK_PERIOD_MS);
      gpio_set_level(MyApp::get().getLEDForButton(ButtonMap[*i]),0);
   }
}

void SimonSaysMultiMenu::showAll() {
   float angleInc = static_cast<float>(2*3.14/NumColors);
   float angle = (3.14/180.0f)*75;
   float angle1Inc = (3.14/360.0f);
   float RadiusOfCircle = 120.0f;
   double px,py;
   for(int i=0;i<NumColors;++i) {
      for(float a=angle;a<angle+angleInc;a+=angle1Inc) {
         px = StartingX + RadiusOfCircle * std::cos(a);
         py = StartingY + RadiusOfCircle * std::sin(a);
         MyApp::get().getDisplay().drawLine(StartingX,StartingY,px,py,Colors[i]);
      }
      angle += angleInc;
   }
}

static uint64_t lastTime = libesp::FreeRTOS::getTimeSinceStart();

ErrorType SimonSaysMultiMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getButtonMgr().addObserver(QueueHandle);
   IState = INIT;
   Sequence.clear();
   addColorToSequence();
   Position = 0;
   lastTime = libesp::FreeRTOS::getTimeSinceStart();

	return ErrorType();
}

static int16_t InputButton = -1;

BaseMenu::ReturnStateContext SimonSaysMultiMenu::onRun() {
   ErrorType et;
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
   gpio_num_t button = GPIO_NUM_NC;
	if(xQueueReceive(QueueHandle, &bme, 0)) {
      if(bme->wasReleased()) {
         button = bme->getButton();
         gpio_set_level(MyApp::get().getLEDForButton(bme->getButton()),0);
      } else {
         gpio_set_level(MyApp::get().getLEDForButton(bme->getButton()),1);
      }
      delete bme;
   }
   nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
      , "Under Development", 5000);
#if 0
   //ServerContext.acceptConnections();
   //ServerContext.readConnections();
   switch(IState) {
      case INIT:
         if(libesp::FreeRTOS::getTimeSinceStart()-lastTime > 1000) {
            MyApp::get().turnOffAllLEDs();
            if(Position >= NumColors) {
               Position = 0;
               lastTime = libesp::FreeRTOS::getTimeSinceStart();
               IState = START_SERVER;
            } else {
               lastTime = libesp::FreeRTOS::getTimeSinceStart();
               MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
               drawWedge(Position);
               gpio_set_level(MyApp::get().getLEDForButton(ButtonMap[Position]),1);
               ++Position;
            }
         }
         break;
      case START_SERVER: {
            MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
            //start listening socket
            //et = ServerContext.start();
            if(et.ok()) {
               IState = ACCEPT_PLAYERS;
            } else {
               nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState()
                     , "failed to start server", 3000);
            }
         }
         break;
      case ACCEPT_PLAYERS: {
            MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
            char buf[32];
            snprintf(buf,sizeof(buf),"ServerID: %s", MyApp::get().getBadgeID());
            //MyApp::get().getDisplay().drawString(35,90, &buf[0],RGBColor::WHITE,RGBColor::BLACK,2);
            //snprintf(buf,sizeof(buf),"Players: %u", ServerContext.getNumConnections());
            //MyApp::get().getDisplay().drawString(35,110,&buf[0],RGBColor::WHITE,RGBColor::BLACK,2);
            //MyApp::get().getDisplay().drawString(35,130,"Press bot to start game",RGBColor::WHITE,RGBColor::BLACK,2);
            if(button==PIN_NUM_BOT_BTN) {
               IState = PLAY_SEQUENCE;
               lastTime = libesp::FreeRTOS::getTimeSinceStart();
            }
         }
         break;
      case PLAY_SEQUENCE:
         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
         playSequence();
         InputButton = -1;
         IState = TAKE_INPUT;
         break;
      case TAKE_INPUT:
         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
         if(button != GPIO_NUM_NC) {
            ++InputButton;
            if(ButtonMap[Sequence[InputButton]] == button) {
               if(InputButton == Position) {
                  if(Position >= MAX_SEQUENCE) {
                     IState = WINNER;
                  } else {
                     IState = MESSAGE;
                     lastTime = libesp::FreeRTOS::getTimeSinceStart();
                     ++Position;
                  }
               }
            } else {
               IState = SHUTDOWN;
            }
         }
         break;
      case MESSAGE:
         MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
         char buf[32];
         sprintf(buf,"Start Round: %d",Position+1);
         MyApp::get().getDisplay().drawString(40,100,buf,RGBColor::WHITE,RGBColor::BLACK,2,false);
         if(libesp::FreeRTOS::getTimeSinceStart()-lastTime > 1500) {
            lastTime = libesp::FreeRTOS::getTimeSinceStart();
            IState = PLAY_SEQUENCE;
         }
         break;
      case WINNER:
         nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), "WINNER!!!", 5000);
         break;
      case SHUTDOWN:
         nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), "Try Again", 3000);
         break;
      case START_CLIENT:
         break;
   }
#endif
	return ReturnStateContext(nextState);
}

ErrorType SimonSaysMultiMenu::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(QueueHandle);
	return ErrorType();
}


