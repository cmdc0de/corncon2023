#ifndef RENDERER_H
#define RENDERER_H

#include <math/vec_math.h>
#include "device/display/color.h"

namespace libesp {
	class BitArray;
	class DisplayDevice;
}

extern libesp::Matrix ModelView;
extern libesp::Matrix Viewport;
extern libesp::Matrix Projection;

struct VertexStruct {
  libesp::Vec3f pos;
	libesp::RGBColor color;
  libesp::Vec3f normal;
	//VertexStruct(const Vec3f &p, const RGBColor &r) : pos(p), color(r) {}
};


class Model {
	//no model matrix to save space and computation we assume identity
public:
	enum MODEL_FORMAT {
		VERTS,
		STRIPS
	};
	Model();
	void set(const VertexStruct *v, uint16_t nv, const uint16_t *i, uint16_t ni, MODEL_FORMAT format);
	const libesp::Vec3f &normal(uint16_t face, uint8_t nVert) const;
	const libesp::Vec3f &vert(uint16_t face, uint8_t nVert) const;
	uint32_t nFaces() const;
	const libesp::Matrix &getModelTransform() const {return ModelTransform;}
	void setTransformation(float t) {ModelTransform.setRotation(t);}
	void scale(float t) {ModelTransform.scale(t);}
private:
	const VertexStruct *Verts;
	uint16_t NumVerts;
	const uint16_t *Indexes;
	uint8_t NumIndexes;
  libesp::Matrix ModelTransform;
	MODEL_FORMAT Format;
};


class IShader {
public:
	IShader();
	virtual ~IShader() = 0;
	virtual libesp::Vec3i vertex(const libesp::Matrix &ModelViewProj, const Model &model, int iface, int nthvert) = 0;
	virtual bool fragment(libesp::Vec3f bar, libesp::RGBColor &color) = 0;
	void setLightDir(const libesp::Vec3f &ld);
	const libesp::Vec3f &getLightDir() const;
private:
  libesp::Vec3f LightDir;
};

class FlatShader: public IShader {
public:
	FlatShader();
	virtual ~FlatShader();

	virtual libesp::Vec3i vertex(const libesp::Matrix &ModelViewProj, const Model &model, int iface, int nthvert);
	virtual bool fragment(libesp::Vec3f bar, libesp::RGBColor &color);
private:
  libesp::mat<3, 3, float> varying_tri;
};

struct GouraudShader: public IShader {
public:
	GouraudShader();
	virtual ~GouraudShader();
	virtual libesp::Vec3i vertex(const libesp::Matrix &ModelViewProj, const Model &model, int iface, int nthvert);
	virtual bool fragment(libesp::Vec3f bar, libesp::RGBColor &color);
private:
  libesp::mat<3, 3, float> varying_tri;
  libesp::Vec3f varying_ity;
};

class ToonShader: public IShader {
public:
	ToonShader();
	virtual ~ToonShader();
	virtual libesp::Vec3i vertex(const libesp::Matrix &ModelViewProj, const Model &model, int iface, int nthvert);
	virtual bool fragment(libesp::Vec3f bar, libesp::RGBColor &color);
private:
  libesp::mat<3, 3, float> varying_tri;
  libesp::Vec3f varying_ity;

};

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); // coeff = -1/c
void lookat(const libesp::Vec3f &eye, const libesp::Vec3f &center, const libesp::Vec3f &up);
//void triangle(Vec3i *pts, IShader &shader, BitArray &zbuffer, DisplayST7735 *display);
void triangle(libesp::Vec3i *pts, IShader &shader, libesp::BitArray &zbuffer, libesp::DisplayDevice *display, const libesp::Vec2i &bboxmin
    , const libesp::Vec2i &bboxmax, uint16_t canvasWdith);

template<typename T> T CLAMP(const T& value, const T& low, const T& high) {
	return value < low ? low : (value > high ? high : value);
}

#endif
