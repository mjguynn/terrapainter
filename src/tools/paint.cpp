#include <imgui/imgui.h>

#include "canvas_tools.h"
#include "../shadermgr.h"
#include "../helpers.h"

// This is a kind of a misnomer. The user can override this.
// This is really just the max for the UI widget.
constexpr float MAX_BRUSH_RADIUS = 256.0f;
class PaintTool : public virtual ICanvasTool {
	// The dimensions of the current canvas.
	ivec2 mCanvasSize;

	vec4 mBrushColor;
	float mBrushRadius;
	float mBrushHardness;

	bool mInStroke;
	vec2 mLastBrushPos;

	// One-channel "mask" storing the current stroke's shape
	GLuint mStrokeTexture;
	// Compute shader used for drawing the stroke into the texture
	GLuint mStrokeProgram;
	// Compute shader using for compositing the stroke onto the canvas
	GLuint mCompositeProgram;
	// Screenspace fragment shader used for drawing the stroke preview
	GLuint mPreviewProgram;

public:
	PaintTool() {
		// We have sensible defaults for these
		mBrushColor = vec4::splat(1);
		mBrushRadius = 20.0f;
		mBrushHardness = 1.0f;
		// We can't do anything about these until we receive canvas info
		// We can still *make* the stroke texture, though...
		mCanvasSize = ivec2::zero();
		glGenTextures(1, &mStrokeTexture);
		configure_texture(mStrokeTexture, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_R8, GL_RED);
		mInStroke = false;
		mLastBrushPos = vec2::zero();
		// Since these go through the shader manager, we don't have to
		// worry about resource cleanup...
		mStrokeProgram = g_shaderMgr.compute("paint_stroke");
		mCompositeProgram = g_shaderMgr.compute("paint_composite");
		mPreviewProgram = g_shaderMgr.screenspace("paint_preview");
	}

	PaintTool(const PaintTool&) = delete;
	PaintTool& operator= (const PaintTool&) = delete;
	PaintTool(const PaintTool&&) = delete;
	PaintTool& operator= (const PaintTool&&) = delete;

	~PaintTool() override {
		assert(mStrokeTexture);
		glDeleteTextures(1, &mStrokeTexture);
	}
	const char* name() const override {
		return "Paint";
	}
	void clear_stroke(ivec2 canvasSize) override {
		mInStroke = false;
		assert(canvasSize.x > 0 && canvasSize.y > 0);
		// We only re-create the texture if the canvas size changed,
		// otherwise we just clear it...
		if (mCanvasSize != canvasSize) {
			glBindTexture(GL_TEXTURE_2D, mStrokeTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, canvasSize.x, canvasSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			mCanvasSize = canvasSize;
		}
		const uint8_t zero = 0;
		glClearTexImage(mStrokeTexture, 0, GL_RED, GL_UNSIGNED_BYTE, &zero);
	}
	bool understands_param(SDL_Keycode keyCode) override {
		return (keyCode == SDLK_f) || (keyCode == SDLK_h) || (keyCode == SDLK_a);
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
		else if (keyCode == SDLK_a) {
			float wanted = mBrushColor.w + 0.01 * (mouseDelta.x + mouseDelta.y);
			mBrushColor.w = std::clamp(wanted, 0.0f, 1.0f);
		}
	}
	void update_stroke(vec2 canvasMouse, bool modifier) override {
		if (!mInStroke) {
			mInStroke = true;
			mLastBrushPos = canvasMouse;
		}
		CanvasRegion start(mLastBrushPos, mBrushRadius);
		CanvasRegion end(canvasMouse, mBrushRadius);
		CanvasRegion total = CanvasRegion::merge(start, end);
		ivec2 size = total.max - total.min;
		// TODO compute offset
		// TODO compute region size
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
		glUseProgram(mCompositeProgram);
		// NOTE: layouts are hardcoded in the shader
		glBindImageTexture(0, mStrokeTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
		glBindImageTexture(1, src, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(2, dst, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glUniform4fv(3, 1, mBrushColor.data());
		glDispatchCompute( (mCanvasSize.x + 15) / 16, (mCanvasSize.y + 15) / 16, 1 );
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	void run_ui() override {
		ImGui::DragFloat("Radius", &mBrushRadius, 1.0f, 1.0f, MAX_BRUSH_RADIUS, "%g");
		ImGui::DragFloat("Hardness", &mBrushHardness, 0.1f, 0.0f, 4.0f, "%.2f");
		ImGui::ColorPicker4("Color", mBrushColor.data());
	}
	void preview(ivec2 screenSize, ivec2 screenMouse, float canvasScale) override {
		// TODO: On Intel:
		// >	API_ID_RECOMPILE_FRAGMENT_SHADER performance warning has been generated. 
		// >	Fragment shader recompiled due to state change.
		// Can't figure out what this means, it doesn't seem to be causing any issues.
		g_shaderMgr.begin_screenspace(mPreviewProgram);
		// NOTE: Layouts are hardcoded in the shader
		glUniform2i(0, screenMouse.x, screenMouse.y);
		glUniform4f(1, 0.0f, mBrushRadius, mBrushHardness, canvasScale);
		glUniform4fv(2, 1, mBrushColor.data());
		g_shaderMgr.end_screenspace();
	}
};

std::unique_ptr<ICanvasTool> tools::paint() {
	return std::make_unique<PaintTool>();
}