#include <imgui/imgui.h>
#include <nfd.hpp>
#include <stb/stb_image.h>

#include "canvas_tools.h"
#include "../shadermgr.h"
#include "../helpers.h"

// This is a kind of a misnomer. The user can override this.
// This is really just the max for the UI widget.
constexpr float MAX_BRUSH_RADIUS = 256.0f;
class SplatterTool : public virtual ICanvasTool {
	// The dimensions of the current canvas.
	ivec2 mCanvasSize;
	GLuint mBufferTexture;

	GLuint mSplatTexture; // TODO: making this an array of textures would be cool
	ivec2 mSplatSize;
	vec4 mSplatTint;
	float mSplatScale;
	float mSplatScaleRnd; // random offset to radius - always positive?
	float mSplatRotation; // radians
	float mSplatRotationRnd; // random offset to rotation, in [0, 2pi]
	float mSplatOffsetRnd; // random offset distance from cursor center
	int mSplatRate; // number of splats per second

	bool mInStroke;

	GLuint mQuadVAO;
	GLuint mQuadVBO;

	GLuint mCompositeProgram;
	GLuint mRenderProgram; // used for rendering splats into buffer & rendering preview

	// pixels: rgba8
	void set_stroke_texture(ivec2 size, uint8_t* pixels) {
		assert(size.x > 0 && size.y > 0);
		// we don't really need this assert idk
		assert(size.x <= MAX_CANVAS_AXIS && size.y <= MAX_CANVAS_AXIS);
		glBindTexture(GL_TEXTURE_2D, mSplatTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture(GL_TEXTURE_2D, 0);
		mSplatSize = size;
	}
	void prompt_choose_stroke_texture() {
		// TODO: This has a lot in common with the canvas prompt
		nfdu8filteritem_t filters[1] = { { "Images", "png,jpg,tga,bmp,psd,gif" } };
		NFD::UniquePathU8 path = nullptr;
		auto res = NFD::OpenDialog(path, filters, 1);
		if (res == NFD_ERROR) {
			fprintf(stderr, "[error] internal error (load dialog)");
		}
		else if (res == NFD_OKAY) {
			ivec2 size;
			stbi_uc* pixels = stbi_load(
				path.get(),
				&size.x,
				&size.y,
				nullptr,
				4);
			if (pixels) set_stroke_texture(size, pixels);
			else fprintf(stderr, "[error] STBI error: %s", stbi_failure_reason());
		}
	}
public:
	SplatterTool() {
		// punt on appropriate canvas size until clear_stroke is called
		mCanvasSize = ivec2::zero();

		glGenTextures(1, &mBufferTexture);
		configure_texture(mBufferTexture, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_RGBA);
		glGenTextures(1, &mSplatTexture);
		configure_texture(mSplatTexture, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_RGBA);
		glGenVertexArrays(1, &mQuadVAO);
		glGenBuffers(1, &mQuadVBO);
		configure_quad(mQuadVAO, mQuadVBO);

		mSplatSize = ivec2::zero();
		mSplatScale = 20.0f;
		mSplatScaleRnd = 0;
		mSplatRotation = 0;
		mSplatRotationRnd = 0;
		mSplatOffsetRnd = 0;
		mSplatRate = 4;
		mInStroke = false;

		mCompositeProgram = g_shaderMgr.compute("splatter_composite");
		mRenderProgram = g_shaderMgr.graphics("simple_2d");
	}

	SplatterTool(const SplatterTool&) = delete;
	SplatterTool& operator= (const SplatterTool&) = delete;
	SplatterTool(const SplatterTool&&) = delete;
	SplatterTool& operator= (const SplatterTool&&) = delete;

	~SplatterTool() override {
		// even if an image isn't loaded (0x0), the texture name should still *exist*
		assert(mSplatTexture);
		glDeleteTextures(1, &mSplatTexture);
		assert(mBufferTexture);
		glDeleteTextures(1, &mBufferTexture);
		assert(mQuadVAO);
		glDeleteVertexArrays(1, &mQuadVAO);
		assert(mQuadVBO);
		glDeleteBuffers(1, &mQuadVBO);
	}
	const char* name() const override {
		return "Splatter";
	}
	void clear_stroke(ivec2 canvasSize) override {
		mInStroke = false;
		assert(canvasSize.x > 0 && canvasSize.y > 0);
		// We only re-create the texture if the canvas size changed,
		// otherwise we just clear it...
		if (mCanvasSize != canvasSize) {
			glBindTexture(GL_TEXTURE_2D, mBufferTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			mCanvasSize = canvasSize;
		}
		const uint8_t zero[4] = { 0, 0, 0, 0 };
		glClearTexImage(mBufferTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, zero);
	}
	bool understands_param(SDL_Keycode keyCode) override {
		return (keyCode == SDLK_f) || (keyCode == SDLK_r) || (keyCode == SDLK_s);
	}
	void update_param(SDL_Keycode keyCode, ivec2 mouseDelta, bool modifier) override {
		if (keyCode == SDLK_f) {
			float wanted = mSplatScale + mouseDelta.x + mouseDelta.y;
			mSplatScale = std::max(wanted, 1.f);
		}
		else if (keyCode == SDLK_r) {
			float wantedRot = mSplatRotation + 0.01 * (mouseDelta.x + mouseDelta.y);
			mSplatRotation = fmodf(wantedRot, 2 * M_PI);
		}
		else if (keyCode == SDLK_s) {
			float wantedSpread = mSplatOffsetRnd + 0.1 * (mouseDelta.x + mouseDelta.y);
			mSplatOffsetRnd = std::max(wantedSpread, 0.f);
		}
	}
	void update_stroke(vec2 canvasMouse, bool modifier) override {

	}
	void composite(GLuint dst, GLuint src) override {
		glUseProgram(mCompositeProgram);
		// NOTE: layouts are hardcoded in the shader
		glBindImageTexture(0, mBufferTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(1, src, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(2, dst, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glUniform4fv(3, 1, mSplatTint.data());
		glDispatchCompute((mCanvasSize.x + 15) / 16, (mCanvasSize.y + 15) / 16, 1);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	void run_ui() override {
		DIAG_PUSHIGNORE_MSVC(4312);
		// this is the way ImGui tells us to cast it
		// manually specifying the UV coordinates since OpenGL's coordinate space is "upside down" compared to ImGui's
		ImGui::Image((ImTextureID)mSplatTexture, ImVec2( 100, 100 ), ImVec2(0, 1), ImVec2(1,0));
		DIAG_POP_MSVC();
		if (ImGui::Button("Choose texture...")) {
			prompt_choose_stroke_texture();
		}
		ImGui::DragFloat("Scale", &mSplatScale, 1.0f, 1.0f, MAX_BRUSH_RADIUS, "%g");
		ImGui::DragFloat("Scale (Random)", &mSplatScaleRnd, 1.0f, 0.0f, MAX_BRUSH_RADIUS, "%g");
		float degRot = mSplatRotation * 180 / M_PI;
		ImGui::DragFloat("Rotation", &degRot, 15.0f, 0.0f, 360.0f, "%g deg");
		mSplatRotation = (degRot * M_PI) / 180;
		float degRotRnd = mSplatRotationRnd * 180 / M_PI;
		ImGui::DragFloat("Rotation (Random):", &degRotRnd, 15.0f, 0.0f, 360.0f, "%g deg");
		mSplatRotationRnd = (degRotRnd * M_PI) / 180;
		ImGui::DragFloat("Offset (Random):", &mSplatOffsetRnd, 1.0f, 0.0f, MAX_BRUSH_RADIUS, "%f");
		ImGui::DragInt("Rate", &mSplatRate, 1.0f, 0, 256);
	}
	void preview(ivec2 screenSize, ivec2 screenMouse, float canvasScale) override {
		// dont even ask how I came up with this
		vec2 nrm = vec2(mSplatSize) / std::max(mSplatSize.x, mSplatSize.y);
		vec2 scale = (canvasScale * mSplatScale * nrm) / vec2(screenSize);
		vec2 pos = (2*vec2(screenMouse) - vec2(screenSize)) / vec2(screenSize);
		auto xform = mat3::translate_hmg(pos)
			* mat2::diag(scale.x, scale.y).hmg()
			* mat2::rotate(mSplatRotation).hmg();
		glBindVertexArray(mQuadVAO);
		glUseProgram(mRenderProgram);
		glUniformMatrix3fv(0, 1, GL_TRUE, xform.data());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mSplatTexture);
		glUniform1i(1, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUseProgram(0);
	}
};

std::unique_ptr<ICanvasTool> tools::splatter() {
	return std::make_unique<SplatterTool>();
}