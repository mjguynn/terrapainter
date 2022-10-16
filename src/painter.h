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
	RGBu8 get_pixel(vec2 coords) const {
		return mPixels[to_offset(coords)];
	}

	void set_pixel(vec2 coords, RGBu8 val) {
		mPixels[to_offset(coords)] = val;
	}

	void process_event(SDL_Event& event);

	// Stub
	void process_ui();

	void draw();

private:

	void draw_circle(vec2 coords, float radius);

	// Converts float X & Y coordinates to an offset into the pixel array
	size_t to_offset(vec2 coords) const {
		assert(0 <= coords.x && coords.x <= 1);
		assert(0 <= coords.y && coords.y <= 1);
		// Row offset: number of rows passed * number of pixels in a row
		size_t row_offset = static_cast<int>(coords.y * mHeight) * mWidth;
		return row_offset + static_cast<int>(coords.x * mWidth);
	}

	vec2 to_coords(int x, int y) const {
		return vec2 {
			float(x) / float(mWidth),
			float(y) / float(mHeight),
		};
	}

	// Invariant: size is width * height
	int mWidth;
	int mHeight;

	// Whether the mouse is down, basically
	bool mDrawing;

	// The current brush radius
	float mRadius;
	
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