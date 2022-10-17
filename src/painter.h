#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "glad/gl.h"
#include "SDL.h"
#include "imgui/imgui.h"
#include "terrapainter/pixel.h"
#include "shaders/screenspace_circle.h"

// NOTE: A lot of this could be refactored into a general image type
class Painter {
public:
	Painter(int width, int height);
	~Painter();

	Painter(const Painter&) = delete;
	Painter& operator= (const Painter&) = delete;

	void process_event(SDL_Event& event, ImGuiIO& io);
	
	void draw();
	void draw_ui();

private:
	class CompositeShader {
		GLuint mProgram;
		GLuint mBlendModeLocation;
		GLuint mBaseTextureLocation;
		GLuint mLayerMaskLocation;
		GLuint mLayerTintLocation;

	public:
		CompositeShader();
		void use(GLuint baseT, GLuint layerT, vec3 layerTint);
	};

	class StrokeShader {
		GLuint mProgram;
		GLuint mSdfLocation;
		GLuint mOriginLocation;
		GLuint mV1Location;
		GLuint mV2Location;
		GLuint mF1Location;
		GLuint mF2Location;
		void submit(GLuint dest, ivec2 origin, ivec2 size, int sdf, ivec2 v1, ivec2 v2, float f1, float f2);
	public:
		StrokeShader();
		void draw_circle(GLuint dest, ivec2 dims, ivec2 center, float radius);
		void draw_rod(GLuint dest, ivec2 dims, ivec2 startPos, ivec2 endPos, float startRadius, float endRadius);
	};

	struct StrokeState {
		// The last position drawn in the current stroke
		ivec2 last_position;
		// The last radius in the current stroke
		float last_radius;
	};

	void commit();

	// Invariant: dims.x * dims.y = mPixels.size();
	ivec2 mDims;

	// Whether currently drawing
	std::optional<StrokeState> mStrokeState;

	// The current brush radius
	float mRadius;
	
	float mRadiusMin;
	float mRadiusMax;

	// The current color
	RGBu8 mColor;

	// The composite shader
	CompositeShader mCompositeS;

	// The stroke-drawing shader
	StrokeShader mStrokeS;

	// The shader program for the UI circle
	ScreenspaceCircleShader mCircleS;

	// The base texture handle
	GLuint mBaseT;

	// The secondary texture handle, used for commits
	GLuint mBufferT;

	// The texture handle for the stroke layer
	GLuint mStrokeT;

	// Framebuffer, used for committing strokes & also during init
	GLuint mFramebuffer;

	// The vertices
	GLuint mVAO;
	GLuint mVBO;
};