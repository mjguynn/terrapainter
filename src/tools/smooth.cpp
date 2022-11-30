#include <imgui/imgui.h>

#include "canvas_tools.h"
#include "../shadermgr.h"
#include "../helpers.h"

class SmoothTool : public virtual ICanvasTool {
	// The dimensions of the current canvas.
	ivec2 mCanvasSize;
	bool mDirtyIntegralTexture;

	// the idea for the integral texture is from
	// https://stackoverflow.com/questions/22436502/how-to-implement-the-gradient-gaussian-blur
	GLuint mIntegralTexture;

	void generate_integral_texture(GLuint src) {
		mDirtyIntegralTexture = false;
	}
public:
	SmoothTool() {
		// punt on appropriate canvas size until clear_stroke is called
		mCanvasSize = ivec2::zero();
		
		glGenTextures(1, &mIntegralTexture);
		configure_texture(mIntegralTexture, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		mDirtyIntegralTexture = false;
	}

	SmoothTool(const SmoothTool&) = delete;
	SmoothTool& operator= (const SmoothTool&) = delete;
	SmoothTool(const SmoothTool&&) = delete;
	SmoothTool& operator= (const SmoothTool&&) = delete;

	~SmoothTool() override {
		assert(mIntegralTexture);
		glDeleteTextures(1, &mIntegralTexture);
	}
	const char* name() const override {
		return "Smooth";
	}
	void clear_stroke(ivec2 canvasSize) override {
		assert(canvasSize.x > 0 && canvasSize.y > 0);
		// We only re-create the texture if the canvas size changed,
		// otherwise we just clear it...
		if (mCanvasSize != canvasSize) {
			glBindTexture(GL_TEXTURE_2D, mIntegralTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_FLOAT, nullptr);
			mCanvasSize = canvasSize;
		}
		// don't bother clearing the integral texture, we'll populate it as soon as we enter stroke
		mDirtyIntegralTexture = true;
	}
	bool understands_param(SDL_Keycode keyCode) override {
		return false;
	}
	void update_param(SDL_Keycode keyCode, ivec2 mouseDelta, bool modifier) override {
	}
	void update_stroke(vec2 canvasMouse, bool modifier) override {
	}
	void composite(GLuint dst, GLuint src) override {
		// This relies on the fact that src is constant throughout a stroke
		// I never documented this anywhere because I don't want to make this guarantee
		// and if we had more time I would refactor the tool interface
		// but for right now, hacks it is!
		if (mDirtyIntegralTexture) generate_integral_texture(src);
	}
	void run_ui() override {
	}
	void preview(ivec2 screenSize, ivec2 screenMouse, float canvasScale) override {
	}
};

std::unique_ptr<ICanvasTool> tools::smooth() {
	return std::make_unique<SmoothTool>();
}