#include <stdlib.h>
#include <device/display/display_device.h>
#include "3d/renderer.h"
#include "device/display/color.h"
#include "menu3d.h"
#include "menu_state.h"
#include "../app.h"
#include "freertos.h"

using libesp::RGBColor;
using libesp::FreeRTOS;
using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::Vec3i;
using libesp::Matrix;
using libesp::Vec3f;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[Menu3D::QUEUE_SIZE*Menu3D::MSG_SIZE] = {0};

static const VertexStruct Cube[] = {
	{	{	-.8, -.8, .8}, 	{0, 0, 0}, {0.0,0.0,0.0}},
	{	{	.8, -.8, .8}, 	   {0, 0, 0}, {0.0,0.0,0.0}},
	{	{	-.8, .8, .8}, 	   {0, 0, 0}, {0.0,0.0,0.0}},
	{	{	.8, .8, .8}, 	   {0, 0, 0}, {0.0,0.0,0.0}},
	{	{	-.8, -.8, -.8}, 	{0, 0, 0}, {0.0,0.0,0.0}},
	{	{	.8, -.8, -.8}, 	{0, 0, 0}, {0.0,0.0,0.0}},
	{	{	-.8, .8, -.8}, 	{0, 0, 0}, {0.0,0.0,0.0}},
	{	{	.8, .8, -.8}, 	   {0, 0, 0}, {0.0,0.0,0.0}}
};

static const uint16_t CubeIndexes[] = {
	0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1
};


const Vec3f Menu3D::center(0, 0, 0);
const Vec3f Menu3D::up(0, 1, 0);

Menu3D::Menu3D(uint8_t w, uint8_t h) : AppBaseMenu(),
		model(), light_dir(), eye(), CanvasWidth(w), CanvasHeight(h) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

Menu3D::~Menu3D() {

}

enum INTERNAL_STATE {
	INIT, RENDER
};

static INTERNAL_STATE InternalState = INIT;
static float rotation = 0.0f;

ErrorType Menu3D::onInit() {
	InternalState = INIT;
	model.set(&Cube[0], (sizeof(Cube) / sizeof(Cube[0])), &CubeIndexes[0]
      , sizeof(CubeIndexes) / sizeof(CubeIndexes[0]), Model::STRIPS);
	light_dir = Vec3f(1, 1, 1);
	eye = Vec3f(0, 2, 40);
	rotation = 0.0f;
	MyApp::get().getButtonMgr().addObserver(InternalQueueHandler);
	return ErrorType();
}

void Menu3D::initMenu3d() {
	lookat(eye, center, up);
	viewport((CanvasWidth / 8), (CanvasHeight / 8), CanvasWidth * 0.7, CanvasHeight * 0.7);
	projection(-1.f / (eye - center).norm());
	light_dir.normalize();
	model.scale(.70);
	model.translateX(0.25f);
	model.translateY(-0.25f);
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
}

BaseMenu::ReturnStateContext Menu3D::onRun() {
	BaseMenu *nextState = this;
   ButtonManagerEvent *bme = nullptr;
	if(xQueueReceive(InternalQueueHandler, &bme, 0)) {
      switch(bme->getButton()) {
         default:
		      if(bme->wasReleased()) nextState = MyApp::get().getMenuState();
            break;
      }
      delete bme;
   }

	switch (InternalState) {
		case INIT:
			initMenu3d();
			InternalState = RENDER;
			break;
		case RENDER:
			update();
			render();
			break;
	}
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType Menu3D::onShutdown() {
	MyApp::get().getButtonMgr().removeObserver(InternalQueueHandler);
	return ErrorType();
}

static uint32_t renderTime = 0, count = 0, total_frames = 0;

void Menu3D::update() {
	model.setTransformation(rotation);
	rotation += 0.05f;
}

inline void swap(int &x, int &y) {
	int tmp = x;
	x = y;
	y = tmp;
}

void Menu3D::line(int x0, int y0, int x1, int y1, RGBColor& color) {
	bool steep = false;
	if (abs(x0 - x1) < abs(y0 - y1)) {
		swap(x0, y0);
		swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	for (int x = x0; x <= x1; x++) {
		float t = (x - x0) / (float) (x1 - x0);
		int y = y0 * (1. - t) + y1 * t;
		if (x >= 0 && x < CanvasWidth && y >= 0 && y < CanvasHeight) {
			if (steep) {
				MyApp::get().getDisplay().drawPixel(y, x, color);
			} else {
				MyApp::get().getDisplay().drawPixel(x, y, color);
			}
		}
	}
}

static uint32_t LastColorChange = 0;
static RGBColor LastColor = RGBColor::WHITE;
static char buf[24];

void Menu3D::render() {
	MyApp::get().getDisplay().fillRec(0, 0,CanvasWidth,CanvasHeight,RGBColor::BLACK);

	RGBColor c = LastColor;
   uint32_t now = FreeRTOS::getTimeSinceStart();
   if(0==LastColorChange) {
      LastColorChange = now;
   }
   if((now-LastColorChange)>1000) {
      LastColorChange = now;
      LastColor = RGBColor(rand()%256,rand()%256,rand()%256);
   }
	FlatShader shader;
	Matrix modelViewProj = Projection * ModelView * model.getModelTransform();
	shader.setLightDir(light_dir);
	//Vec3i screen_coords[3];
	for (uint32_t i = 0; i < model.nFaces(); i++) {
		for (int j = 0; j < 3; j++) {
			Vec3i v0 = shader.vertex(modelViewProj, model, i, j);
			Vec3i v1 = shader.vertex(modelViewProj, model, i, (j + 1) % 3);
			line(v0.x, v0.y, v1.x, v1.y, c);
		}
	}
	if (FreeRTOS::getTimeSinceStart() - renderTime > 1000) {
		sprintf(&buf[0], "FPS: %u:%u", count, total_frames);
		count = 0;
		renderTime = FreeRTOS::getTimeSinceStart();
	}
   MyApp::get().getDisplay().drawString(70, 180, &buf[0]);
	++count;
	total_frames++;
}

