#pragma once

#include <optional>
#include <vector>
#include "glad/gl.h"
#include "SDL.h"
#include "imgui/imgui.h"
#include "terrapainter/pixel.h"
#include "shaders/screenspace_circle.h"

class Canvas {
public:
	Canvas(int width, int height);
	~Canvas();

	Canvas(const Canvas&) = delete;
	Canvas& operator= (const Canvas&) = delete;

	void process_event(SDL_Event& event, ImGuiIO& io);

	ivec2 dimensions() const {
		return mDims;
	}

	// Retrieve the current texture from the GPU.
	// This is an expensive operation.
	std::vector<RGBu8> dump_texture() const;

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
		void use(int mBlendMode, GLuint baseT, GLuint layerT, vec3 layerTint);
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

	enum BlendMode {
		Mix = 0,
		Add = 1,
		Sub = 2
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

	// The current color (0.0-1.0)
	vec3 mColor;

	// The current blend mode
	int mBlendMode;

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