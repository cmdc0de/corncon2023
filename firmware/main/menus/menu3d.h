#ifndef MENU3D_H
#define MENU3D_H

#include "appbase_menu.h"
#include "3d/renderer.h"
#include <math/vec_math.h>

namespace libesp {
	class DisplayDevice;
	class RGBColor;
}


class Menu3D : public AppBaseMenu {
public:
	Menu3D(uint8_t w, uint8_t h);
	virtual ~Menu3D();
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(ButtonManagerEvent*);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	void initMenu3d();
	void update();
	void render();
	void line(int x0, int y0, int x1, int y1, libesp::RGBColor& color);
private:
	Model model;
  libesp::Vec3f light_dir;
  libesp::Vec3f eye;
	static const libesp::Vec3f center;
	static const libesp::Vec3f up;
	uint8_t CanvasWidth;
	uint8_t CanvasHeight;
	QueueHandle_t InternalQueueHandler;
};

#endif
