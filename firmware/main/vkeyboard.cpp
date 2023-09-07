#include "vkeyboard.h"
#include <string.h>
#include <device/display/display_device.h>
#include "app.h"

static const char *LOGTAG = "VKeyBoard";

VKeyboard::VKeyboard() : Position(0), KeyBoard(0), SizeOfKeyBoard(0)
   , NumToDraw(0) {
}

libesp::ErrorType VKeyboard::init(const char* keyBoard, uint16_t numToDraw) {
   libesp::ErrorType et;
   NumToDraw = numToDraw;
   if((NumToDraw&0x1)==0) {
      et = libesp::ErrorType::INVALID_PARAM;
   } else {
      KeyBoard = keyBoard;
      SizeOfKeyBoard = strlen(KeyBoard);
      Position = 0;
   }
   return et;
}

void VKeyboard::moveRight() {
   Position==(SizeOfKeyBoard-1)?0:++Position;
}

void VKeyboard::moveLeft() {
   Position==0?SizeOfKeyBoard-1:--Position;
}

void VKeyboard::reset() {
   Position = 0;
}

libesp::ErrorType VKeyboard::draw(MyApp::AppDisplayType &d, uint16_t x, uint16_t y) {
   libesp::ErrorType et;
   uint16_t half = NumToDraw/2;
   uint16_t width = d.getFont()->FontWidth*half;
   uint16_t height = d.getFont()->FontHeight/2;
   // ESP_LOGI(LOGTAG,"half %d - width %d",half,width);
   if(Position>=half && Position<(SizeOfKeyBoard-half)) {
      d.drawString(x, y, &KeyBoard[Position-half], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 1, false, half);
      x = x + width + d.getFont()->FontWidth;
      d.drawString(x, y-height, &KeyBoard[Position], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 3, false, 1);
      x = x + d.getFont()->FontWidth*3.5;
      d.drawString(x, y, &KeyBoard[Position+1], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 1, false, half);
   } else if(Position<half) {
      //ESP_LOGI(LOGTAG,"here %d",Position);
      uint16_t ox = x;
      x = x + (half-Position)*d.getFont()->FontWidth;
      if(Position!=0) {
         d.drawString(x, y, &KeyBoard[0], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 1, false, Position);
      }
      x = ox + width + d.getFont()->FontWidth;
      d.drawString(x, y-height, &KeyBoard[Position], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 3, false, 1);
      x = x + d.getFont()->FontWidth*3.5;
      d.drawString(x, y, &KeyBoard[Position+1], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 1, false, half);
   } else if(Position>=SizeOfKeyBoard-half) {
      d.drawString(x, y, &KeyBoard[Position-half], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 1, false, half);
      x = x + width + d.getFont()->FontWidth;
      d.drawString(x, y-height, &KeyBoard[Position], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 3, false, 1);
      x = x + d.getFont()->FontWidth*3.5;
      if(Position!=SizeOfKeyBoard-1) {
         d.drawString(x, y, &KeyBoard[Position+1], libesp::RGBColor::WHITE, libesp::RGBColor::BLACK
            , 1, false, SizeOfKeyBoard-1-Position);
      }
   }
   return et;
}

char VKeyboard::getSelection() {
   return KeyBoard[Position];
}

