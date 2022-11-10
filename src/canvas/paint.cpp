#include "tool.h"
#include "../shadermgr.h"

class PaintTool : ICanvasTool {
	// The dimensions of the current canvas.
	ivec2 mCanvasSize;

	// The color of the brush
	vec3 mBrushColor;
	// mBrushParams.x = brush outer radius (brush opacity is 0)
	// mBrushParams.y = brush inner radius (brush opacity is 1) - stored as percentage
	// mBrushParams.z = brush hardness (how "fast" it goes from 0 to 1)
	vec3 mBrushParams;

	// One-channel "mask" storing the current stroke's shape
	GLuint mStrokeTexture;
	// Compute shader used for drawing the stroke into the texture
	GLuint mStrokeProgram;

	// Fullscreen fragment shader used for drawing the stroke preview
	GLuint mPreviewProgram;

public:
	PaintTool() {
		// We have sensible defaults for these
		mBrushColor = vec3::splat(1);
		mBrushParams = vec3(20.0f, 0.0f, 1.0f);
		// We can't do anything about these until we receive canvas info
		// We can still *make* the stroke texture, though...
		mCanvasSize = ivec2::zero();
		glGenTextures(1, &mStrokeTexture);
		glBindTexture(GL_TEXTURE_2D, mStrokeTexture);
		// GL_LINEAR for min filter sucks, but the "good" alternative would be computing
		// mipmaps each frame... yeah, no, we're sticking with linear.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// We can easily set these up.
		// Since these go through the shader manager, we don't have to
		// worry about resource cleanup...
		mStrokeProgram = g_shaderMgr.compute("paint_stroke");
		mPreviewProgram = g_shaderMgr.compute("paint_preview");
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
	void clear(ivec2 canvasSize) override {
		assert(canvasSize.x > 0 && canvasSize.y > 0);
		// We only re-create the texture if the canvas size changed,
		// otherwise we just clear it...
		if (mCanvasSize != canvasSize) {
			glBindTexture(GL_TEXTURE_2D, mStrokeTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, canvasSize.x, canvasSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			mCanvasSize = canvasSize;
		}
		const float zero = 0.0f;
		glClearTexImage(mStrokeTexture, 0, GL_RED, GL_UNSIGNED_BYTE, &zero);
	}
	void configure(SDL_KeyCode keyCode, ivec2 mouseDelta, bool modifier) override {
		if (keyCode == SDLK_r) {
			// Y axis = brush outer radius
			float wanted = mBrushParams.z + mouseDelta.y;
			mBrushParams.z = std::clamp(wanted, 1.f, 100.f);
		}
		else if (keyCode == SDLK_f) {
			// TODO!
		}
	}
};