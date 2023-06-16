#pragma once

#include <error_type.h>

namespace libesp {
   class DisplayDevice;
}

class VKeyboard {
public:
   static constexpr const char *K1 ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcedfghijklmnopqrstuvwxyz!@#$%^&*0123456789";
   static constexpr const char *N1 ="0123456789";
   static constexpr const char *Y1 ="YN";
public:
   VKeyboard();
   libesp::ErrorType init(const char* keyBoard, uint16_t numToDraw);
   void moveRight();
   void moveLeft();
   void reset();
   libesp::ErrorType draw(libesp::DisplayDevice& d, uint16_t x, uint16_t y);
   char getSelection();
private:
   uint16_t Position;
   const char *KeyBoard;
   uint16_t SizeOfKeyBoard;
   uint16_t NumToDraw;
};
