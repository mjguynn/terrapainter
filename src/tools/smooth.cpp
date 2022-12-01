#include <imgui/imgui.h>

#include "canvas_tools.h"
#include "../shadermgr.h"
#include "../helpers.h"

constexpr float MAX_BRUSH_RADIUS = 256.0f;
class SmoothTool : public virtual ICanvasTool {
	// The dimensions of the current canvas.
	ivec2 mCanvasSize;
	bool mInStroke;
	bool mDirtyIntegralTexture;

	float mBrushRadius;
	float mBrushHardness;
	vec2 mLastBrushPos;
	int mBlurRadius;

	// the idea for the integral texture is from
	// https://stackoverflow.com/questions/22436502/how-to-implement-the-gradient-gaussian-blur
	GLuint mIntegralXTexture; // <-- this is an intermediate texture used for integrating along X dimension...
	GLuint mIntegralXYTexture; // <-- which then gets integrated again, but in the Y direction, to give this
	GLuint mStrokeTexture;

	GLuint mCompositeProgram;
	GLuint mStrokeProgram;
	GLuint mPreviewProgram;
	// Compute shader input images must have a definite type (i.e. rgba8, rgba32f)
	// This means I had to choose one of the following:
	//	1. Make canvas texture RGBA32F
	//	2. Copy canvas texture into RGBA32F texture and implement logic for switching axis at runtime
	//	3. Make seperate X and XY shaders
	// I chose number 3.
	GLuint mIntegralXProgram;
	GLuint mIntegralXYProgram;

	void generate_integral_texture(GLuint src) {
		glUseProgram(mIntegralXProgram);
		glBindImageTexture(0, src, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(1, mIntegralXTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUniform1i(2, mCanvasSize.y);
		glDispatchCompute((mCanvasSize.x + 255) / 256, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(mIntegralXYProgram);
		glBindImageTexture(0, mIntegralXTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, mIntegralXYTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUniform1i(2, mCanvasSize.x);
		glDispatchCompute(1, (mCanvasSize.y + 255) / 256, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		mDirtyIntegralTexture = false;
	}
public:
	SmoothTool() {
		// punt on appropriate canvas size until clear_stroke is called
		mCanvasSize = ivec2::zero();
		mInStroke = false;
		mDirtyIntegralTexture = false;

		mBrushRadius = 20.0f;
		mBrushHardness = 1.0f;
		mLastBrushPos = vec2::zero();
		mBlurRadius = 16;
		glGenTextures(1, &mIntegralXTexture);
		configure_texture(mIntegralXTexture, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		glGenTextures(1, &mIntegralXYTexture);
		configure_texture(mIntegralXYTexture, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		glGenTextures(1, &mStrokeTexture);
		configure_texture(mStrokeTexture, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_R8, GL_RED);

		mCompositeProgram = g_shaderMgr.compute("smooth_composite");
		mStrokeProgram = g_shaderMgr.compute("paint_stroke");
		mPreviewProgram = g_shaderMgr.screenspace("paint_preview");
		mIntegralXProgram = g_shaderMgr.compute("smooth_integrate_x");
		mIntegralXYProgram = g_shaderMgr.compute("smooth_integrate_xy");
	}

	SmoothTool(const SmoothTool&) = delete;
	SmoothTool& operator= (const SmoothTool&) = delete;
	SmoothTool(const SmoothTool&&) = delete;
	SmoothTool& operator= (const SmoothTool&&) = delete;

	~SmoothTool() override {
		assert(mIntegralXTexture);
		glDeleteTextures(1, &mIntegralXTexture);
		assert(mIntegralXYTexture);
		glDeleteTextures(1, &mIntegralXYTexture);
		assert(mStrokeTexture);
		glDeleteTextures(1, &mStrokeTexture);
	}
	const char* name() const override {
		return "Smooth";
	}
	void clear_stroke(ivec2 canvasSize) override {
		mInStroke = false;
		mDirtyIntegralTexture = true;
		assert(canvasSize.x > 0 && canvasSize.y > 0);
		// We only re-create the texture if the canvas size changed,
		// otherwise we just clear it...
		if (mCanvasSize != canvasSize) {
			glBindTexture(GL_TEXTURE_2D, mIntegralXTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_FLOAT, nullptr);
			glBindTexture(GL_TEXTURE_2D, mIntegralXYTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_FLOAT, nullptr);
			glBindTexture(GL_TEXTURE_2D, mStrokeTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, canvasSize.x, canvasSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			mCanvasSize = canvasSize;
		}
		// don't bother clearing the integral texture, we'll populate it as soon as we enter stroke
		const uint8_t zero = 0;
		glClearTexImage(mStrokeTexture, 0, GL_RED, GL_UNSIGNED_BYTE, &zero);
	}
	bool understands_param(SDL_Keycode keyCode) override {
		return (keyCode == SDLK_f) || (keyCode == SDLK_h);
	}
	void update_param(SDL_Keycode keyCode, ivec2 mouseDelta, bool modifier) override {
		if (keyCode == SDLK_f) {
			float wanted = mBrushRadius + mouseDelta.x + mouseDelta.y;
			mBrushRadius = std::max(wanted, 1.f);
		}
		else if (keyCode == SDLK_h) {
			float wanted = mBrushHardness + 0.1 * (mouseDelta.x + mouseDelta.y);
			mBrushHardness = std::clamp(wanted, 0.f, 4.f);
		}
	}
	void update_stroke(vec2 canvasMouse, bool modifier) override {
		// This is copied DIRECTLY from paint.cpp
		if (!mInStroke) {
			mInStroke = true;
			mLastBrushPos = canvasMouse;
		}
		CanvasRegion start(mLastBrushPos, mBrushRadius);
		CanvasRegion end(canvasMouse, mBrushRadius);
		CanvasRegion total = CanvasRegion::merge(start, end);
		ivec2 size = total.max - total.min;
		glUseProgram(mStrokeProgram);
		glBindImageTexture(0, mStrokeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
		glUniform2iv(1, 1, total.min.data());
		glUniform2fv(2, 1, mLastBrushPos.data());
		glUniform2fv(3, 1, canvasMouse.data());
		glUniform2f(4, mBrushRadius, mBrushHardness);
		glDispatchCompute((size.x + 15) / 16, (size.y + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		mLastBrushPos = canvasMouse;
	}
	void composite(GLuint dst, GLuint src) override {
		// This relies on the fact that src is constant throughout a stroke
		// I never documented this anywhere because I don't want to make this guarantee
		// and if we had more time I would refactor the tool interface
		// but for right now, hacks it is!
		if (mDirtyIntegralTexture) generate_integral_texture(src);
		glUseProgram(mCompositeProgram);
		glBindImageTexture(0, mStrokeTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mIntegralXYTexture);
		glUniform1i(1, 0);
		glBindImageTexture(2, src, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(3, dst, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glUniform1i(4, mBlurRadius);
		glDispatchCompute((mCanvasSize.x + 15) / 16, (mCanvasSize.y + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}
	void run_ui() override {
		ImGui::DragFloat("Brush Radius", &mBrushRadius, 1.0f, 1.0f, MAX_BRUSH_RADIUS, "%g");
		ImGui::DragFloat("Brush Hardness", &mBrushHardness, 0.1f, 0.0f, 4.0f, "%.2f");
		ImGui::DragInt("Smooth Size", &mBlurRadius, 1.0f, 1, MAX_BRUSH_RADIUS, "%ipx");
		if (mBlurRadius < 1) mBlurRadius = 1;
	}
	void preview(ivec2 screenSize, ivec2 screenMouse, float canvasScale) override {
		// Again, copied wholesale from paint.cpp, except we have no stroke color...
		g_shaderMgr.begin_screenspace(mPreviewProgram);
		glUniform2i(0, screenMouse.x, screenMouse.y);
		glUniform4f(1, 0.0f, mBrushRadius, mBrushHardness, canvasScale);
		float v = mInStroke ? 0.0f : 1.0f;
		glUniform4f(2, v, v, v, v);
		g_shaderMgr.end_screenspace();
	}
};

std::unique_ptr<ICanvasTool> tools::smooth() {
	return std::make_unique<SmoothTool>();
}