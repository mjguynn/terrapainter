#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <nfd.hpp>

#include "canvas.h"

// TODO: Move this to a common header or something, it's pretty useful
static void configure_texture(GLuint texture, GLenum min, GLenum mag, GLenum sWrap, GLenum tWrap) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
}
Canvas::Canvas() {
	mTools = std::vector<std::unique_ptr<ICanvasTool>>();
	mCurTool = 0;
	mCanvasOffset = ivec2::zero();
	mCanvasScale = 1.0f;
	// We don't know the size or contents of the canvas yet,
	// but we can still register texture objects for them 
	// and resize/fill them as needed.
	mCanvasSize = ivec2::zero();
	glGenTextures(1, &mCanvasTexture);
	configure_texture(mCanvasTexture, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	glGenTextures(1, &mCanvasSwapTexture);
	configure_texture(mCanvasTexture, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}
Canvas::~Canvas() noexcept {
	assert(mCanvasTexture && mCanvasSwapTexture);
	glDeleteTextures(1, &mCanvasTexture);
	glDeleteTextures(1, &mCanvasSwapTexture);
}
ivec2 Canvas::get_canvas_size() const {
	return mCanvasSize;
}
std::vector<uint8_t> Canvas::get_canvas() const {
	size_t numPixels = size_t(mCanvasSize.x) * size_t(mCanvasSize.y);
	std::vector<uint8_t> pixels(numPixels);
	if (numPixels == 0) {
		return pixels;
	}
	pixels.resize(numPixels * 4); // 4 bytes per pixel since RGBA
	glBindTexture(GL_TEXTURE_2D, mCanvasTexture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	return pixels;
}
void Canvas::set_canvas(ivec2 canvasSize, uint8_t* pixels) {
	// Canvas dimensions should be either 0x0 or positive.
	if (canvasSize != ivec2::zero()) {
		assert(canvasSize.x > 0 && canvasSize.y > 0);
		glBindTexture(GL_TEXTURE_2D, mCanvasTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		// Inform tools of the change
		for (auto& tool : mTools) {
			tool->clear(canvasSize);
		}
	}
	mCanvasSize = canvasSize;
}
Canvas::ToolIndex Canvas::register_tool(std::unique_ptr<ICanvasTool> tool) {
	mTools.emplace_back(std::move(tool));
	return mTools.size() - 1;
}
void Canvas::set_current_tool(ToolIndex toolIndex) {
	assert(toolIndex < mTools.size());
	mCurTool = toolIndex;
}
void Canvas::activate() {
	glDepthFunc(GL_ALWAYS);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
}
void Canvas::deactivate() {}
void Canvas::process_event(const SDL_Event& event) {
	// TODO
	if (event.type == SDL_KEYDOWN) {
		const Uint8* keys = SDL_GetKeyboardState(nullptr);
		SDL_Keycode pressed = event.key.keysym.sym;
		bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
		if (ctrl && pressed == SDLK_o) {
			prompt_open();
		}
		else if (ctrl && pressed == SDLK_s) {
			prompt_save();
		}
	}
}
void Canvas::process_frame(float deltaTime) {}
void Canvas::render(ivec2 viewportSize) const {
	// TODO
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
void Canvas::prompt_open() {
	nfdu8filteritem_t filters[1] = { { "Images", "png,jpg,tga,bmp,psd,gif" } };
	NFD::UniquePathU8 path = nullptr;
	auto res = NFD::OpenDialog(path, filters, 1);
	if (res == NFD_ERROR) {
		fprintf(stderr, "[error] internal error (load dialog)");
	}
	else if (res == NFD_OKAY) {
		ivec2 canvasSize;
		stbi_uc* pixels = stbi_load(
			path.get(), 
			&canvasSize.x, 
			&canvasSize.y,
			nullptr,
			4);

		if (pixels) {
			set_canvas(canvasSize, pixels);
		}
		else {
			fprintf(stderr, "[error] STBI error: %s", stbi_failure_reason());
		}
	}
}
void Canvas::prompt_save() const {
	fprintf(stderr, "[info] dumping texture...");
	auto pixels = get_canvas();
	fprintf(stderr, " complete\n");

	nfdu8filteritem_t filters[1] = { { "PNG Images", "png" } };
	NFD::UniquePathU8 path = nullptr;
	auto res = NFD::SaveDialog(
		path,
		filters,
		1,
		nullptr,
		"output.png");

	if (res == NFD_ERROR) {
		fprintf(stderr, "[error] internal error (save dialog)\n");
	}
	else if (res == NFD_OKAY) {
		stbi_write_png(
			path.get(), 
			mCanvasSize.x, 
			mCanvasSize.y, 
			4, 
			pixels.data(), 
			mCanvasSize.x * 4);
		fprintf(stderr, "[info] image saved to \"%s\"\n", path.get());
	}
}
void Canvas::run_ui() {
	// TODO
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "O", nullptr)) {
				prompt_open();
			}
			if (ImGui::MenuItem("Save", "S", nullptr)) {
				prompt_save();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}