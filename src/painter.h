#pragma once

#include <cstdint>
#include <vector>
#include "glad/gl.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "terrapainter/pixel.h"
#include "shaders/unlit.h"

// NOTE: A lot of this could be refactored into a general image type
class Painter {
public:
	Painter(int width, int height);
	~Painter();

	Painter(const Painter&) = delete;
	Painter& operator= (const Painter&) = delete;

	// Uses 0.0-1.0 coordinates
	RGBu8 get_pixel(int x, int y) const {
		return mPixels[to_offset(x, y)];
	}

	void set_pixel(int x, int y, RGBu8 val) {
		mPixels[to_offset(x, y)] = val;
	}

	void process_event(SDL_Event& event);

	// Stub
	void process_ui();

	void draw();

private:

	void draw_circle(int x, int y, float radius, RGBu8 color);

	// Converts float X & Y coordinates to an offset into the pixel array
	size_t to_offset(int x, int y) const {
		assert(0 <= x && x < mWidth);
		assert(0 <= y && y < mHeight);
		assert(mPixels.size() == mWidth * mHeight);
		// Row offset: number of rows passed * number of pixels in a row
		return y * mWidth + x;
	}

	// Invariant: size is width * height
	int mWidth;
	int mHeight;

	// Whether the mouse is down, basically
	bool mDrawing;

	// The current brush radius
	float mRadius;
	
	// The current color
	RGBu8 mColor;

	// The pixel buffer
	std::vector<RGBu8> mPixels;

	// The shader program
	UnlitShader mShader;

	// The texture handle
	GLuint mTexture;

	// The vertices
	GLuint mVAO;
	GLuint mVBO;
};