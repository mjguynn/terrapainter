#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "glad/gl.h"
#include "SDL.h"
#include "imgui/imgui.h"
#include "terrapainter/pixel.h"
#include "shaders/unlit.h"
#include "shaders/screenspace_circle.h"

// NOTE: A lot of this could be refactored into a general image type
class Painter {
public:
	Painter(int width, int height);
	~Painter();

	Painter(const Painter&) = delete;
	Painter& operator= (const Painter&) = delete;

	RGBu8 get_pixel(ivec2 coords) const {
		return mPixels[to_offset(coords)];
	}

	void set_pixel(ivec2 coords, RGBu8 val) {
		mPixels[to_offset(coords)] = val;
	}

	void process_event(SDL_Event& event, ImGuiIO& io);
	
	void draw();
	void draw_ui();

private:
	void draw_circle(ivec2 position, float radius, RGBu8 color);
	void draw_rod(ivec2 start, ivec2 end, float radius, RGBu8 color);

	// Converts float X & Y coordinates to an offset into the pixel array
	size_t to_offset(ivec2 coords) const {
		assert(0 <= coords.x && coords.x < mDims.x);
		assert(0 <= coords.y && coords.y < mDims.y);
		assert(mPixels.size() == size_t(mDims.x) * size_t(mDims.y));
		// Row offset: number of rows passed * number of pixels in a row
		return size_t(coords.y) * mDims.x + coords.x;
	}

	// Invariant: dims.x * dims.y = mPixels.size();
	ivec2 mDims;

	// If currently drawing: contains the starting stroke pixel position
	// If not currently drawing: nullopt
	std::optional<ivec2> mStrokeStart;

	// The current brush radius
	float mRadius;
	
	float mRadiusMin;
	float mRadiusMax;

	// The current color
	RGBu8 mColor;

	// The pixel buffer
	std::vector<RGBu8> mPixels;

	// The shader program
	UnlitShader mShader;

	// The shader program for the UI circle
	ScreenspaceCircleShader mUiCircleShader;

	// The texture handle
	GLuint mTexture;

	// The vertices
	GLuint mVAO;
	GLuint mVBO;

	// Whether the texture has been modified since the last texture upload
	bool mNeedsUpload;
};