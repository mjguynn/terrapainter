#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <nfd.hpp>

#include "shadermgr.h"
#include "canvas.h"
#include "helpers.h"

Canvas::Canvas(SDL_Window* window) {
	mTools = std::vector<std::unique_ptr<ICanvasTool>>();
	mCurTool = 0;
	mCanvasOffset = ivec2::zero();
	mCanvasScaleLog = 0.0f;
	// We don't know the size or contents of the canvas yet,
	// but we can still register texture objects for them 
	// and resize/fill them as needed.
	mCanvasSize = ivec2::zero();
	glGenTextures(1, &mCanvasTexture);
	configure_texture(mCanvasTexture, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_RGBA);
	glGenTextures(1, &mCanvasDstTexture);
	configure_texture(mCanvasDstTexture, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_RGBA);
	mCanvasProgram = g_shaderMgr.graphics("simple_2d");
	glGenVertexArrays(1, &mCanvasVAO);
	glGenBuffers(1, &mCanvasVBO);
	configure_quad(mCanvasVAO, mCanvasVBO);
	mHeldKey = SDLK_0; // doesn't matter
	mLastMousePos = ivec2::zero(); // doesn't matter
	mWindow = window;
	mModified = false;
	mInteractState = InteractState::NONE;
	mPath = std::filesystem::path();
	mShowNewDialog = false; // TODO change this?
	mNewDialogCanvasSize = ivec2{ 512, 512 }; // seems reasonable
	mDidAStupid = false;
}
Canvas::~Canvas() noexcept {
	assert(mCanvasTexture);
	glDeleteTextures(1, &mCanvasTexture);
	assert(mCanvasDstTexture);
	glDeleteTextures(1, &mCanvasDstTexture);
	assert(mCanvasVAO);
	glDeleteVertexArrays(1, &mCanvasVAO);
	assert(mCanvasVBO);
	glDeleteBuffers(1, &mCanvasVBO);
}
ivec2 Canvas::get_canvas_size() const {
	return mCanvasSize;
}
GLuint Canvas::get_canvas_texture() const {
	return mCanvasTexture;
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
bool Canvas::set_canvas(ivec2 canvasSize, uint8_t* pixels, std::filesystem::path source) {
	if (canvasSize.x < 0 || canvasSize.y < 0)
		return false;
	if (canvasSize.x > MAX_CANVAS_AXIS || canvasSize.y > MAX_CANVAS_AXIS)
		return false;
	if (canvasSize != ivec2::zero()) {
		glBindTexture(GL_TEXTURE_2D, mCanvasTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		if(!pixels){
			// that image just got filled with uninitialized memory, let's fix this.
			uint8_t clearColor[4] = { 0, 0, 0, 255 };
			glClearTexImage(mCanvasTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, clearColor);
		}
		// Also resize the dst texture, this is overwritten by composite so we don't care about giving it sensible values
		// We can just leave it uninitialized
		glBindTexture(GL_TEXTURE_2D, mCanvasDstTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		// Inform tools of the change
		for (auto& tool : mTools) {
			tool->clear_stroke(canvasSize);
		}
	}
	mCanvasSize = canvasSize;
	mModified = false;
	mShowNewDialog = false;
	mPath = source;
	if (mPath.empty()) {
		SDL_SetWindowTitle(mWindow, "Terrapainter");
	}
	else {
		auto title = "Terrapainter - " + mPath.filename().string();
		SDL_SetWindowTitle(mWindow, title.c_str());
	}
	return true;
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
	SDL_SetRelativeMouseMode(mInteractState == InteractState::CONFIGURE ? SDL_TRUE : SDL_FALSE);
}
// NOTE: setting state to stroke doesn't create initial stroke
// NOTE: setting state to configure doesn't set the held key
void Canvas::set_interact_state(InteractState s) {
	if (mInteractState == s) return;
	
	// Transition to InteractState::NONE, do cleanup as necessary
	if (mInteractState == InteractState::NONE) {
		// Nothing needs to be done
	}
	else if (mInteractState == InteractState::PAN) {
		// Nothing needs to be done
	}
	else if (mInteractState == InteractState::STROKE) {
		// Commit current stroke, clear canvas
		mTools.at(mCurTool)->composite(mCanvasDstTexture, mCanvasTexture);
		mTools.at(mCurTool)->clear_stroke(mCanvasSize);
		std::swap(mCanvasDstTexture, mCanvasTexture);
		mModified = true;
	}
	else if (mInteractState == InteractState::CONFIGURE) {
		// Reset mouse state
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_WarpMouseInWindow(mWindow, mLastMousePos.x, mLastMousePos.y);
	}

	// Transition to desired state
	mInteractState = s;
	if (mInteractState == InteractState::NONE) {
		// Nothing needs to be done
	}
	else if (mInteractState == InteractState::PAN) {
		// Nothing needs to be done
	} 
	else if (mInteractState == InteractState::STROKE) {
		// Nothing really needs to be done
		// We allow transitioning to the stroke state without placing an initial stroke
	}
	else if (mInteractState == InteractState::CONFIGURE) {
		// You must set mHeldKey first
		// I wish C++ had rust enums for this :(
		SDL_GetMouseState(&mLastMousePos.x, &mLastMousePos.y);
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
}
void Canvas::deactivate() {
	set_interact_state(InteractState::NONE);
}
void Canvas::process_key_down(const SDL_KeyboardEvent& event) {
	const Uint8* keys = SDL_GetKeyboardState(nullptr);
	bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
	if (ctrl && event.keysym.sym == SDLK_n) {
		prompt_new();
	}
	else if (ctrl && event.keysym.sym == SDLK_s) {
		prompt_save();
	}
	else if (ctrl && event.keysym.sym == SDLK_o) {
		prompt_open();
	}
	else if (mInteractState == InteractState::NONE && !mTools.empty()){
		if (!mTools.at(mCurTool)->understands_param(event.keysym.sym))
			return;
		mHeldKey = event.keysym.sym;
		set_interact_state(InteractState::CONFIGURE);
	}
}
void Canvas::process_key_up(const SDL_KeyboardEvent& event) {
	if (mInteractState == InteractState::CONFIGURE && mHeldKey == event.keysym.sym) {
		set_interact_state(InteractState::NONE);
	}
}
void Canvas::process_mouse_button_down(const SDL_MouseButtonEvent& event) {
	int w, h;
	SDL_GetWindowSizeInPixels(mWindow, &w, &h);
	ivec2 pos = { event.x, h - event.y };
	const Uint8* keys = SDL_GetKeyboardState(nullptr);
	if (mInteractState == InteractState::NONE) {
		if (event.button == SDL_BUTTON_LEFT && !mTools.empty()) {
			vec2 canvasMouse = cursor_canvas_coords();
			mTools.at(mCurTool)->update_stroke(canvasMouse, keys[SDL_SCANCODE_LSHIFT]);
			set_interact_state(InteractState::STROKE);
		}
		else if (event.button == SDL_BUTTON_RIGHT) {
			set_interact_state(InteractState::PAN);
		}
	}
}
void Canvas::process_mouse_button_up(const SDL_MouseButtonEvent& event) {
	if ( mInteractState == InteractState::PAN && event.button == SDL_BUTTON_RIGHT) {
		set_interact_state(InteractState::NONE);
	}
	else if (mInteractState == InteractState::STROKE && event.button == SDL_BUTTON_LEFT) {
		set_interact_state(InteractState::NONE);
	}
}
void Canvas::process_mouse_motion(const SDL_MouseMotionEvent& event) {
	const Uint8* keys = SDL_GetKeyboardState(nullptr);
	ivec2 delta = { event.xrel, -event.yrel };
	if (mInteractState == InteractState::PAN) {
		mCanvasOffset += delta;
	}
	else if (mInteractState == InteractState::STROKE) {
		vec2 canvasMouse = cursor_canvas_coords();
		mTools.at(mCurTool)->update_stroke(canvasMouse, keys[SDL_SCANCODE_LSHIFT]);
	}
	else if (mInteractState == InteractState::CONFIGURE) {
		mTools.at(mCurTool)->update_param(mHeldKey, delta, keys[SDL_SCANCODE_LSHIFT]);
	}
}
void Canvas::process_mouse_wheel(const SDL_MouseWheelEvent& event) {
	int w, h;
	SDL_GetWindowSizeInPixels(mWindow, &w, &h);
	float oldScale = powf(2.0f, mCanvasScaleLog);
	mCanvasScaleLog += event.preciseY;
	float newScale = powf(2.0f, mCanvasScaleLog);
	float fac = (newScale / oldScale) - 1;
	// cursor pos in window pixels relative to center of the image
	ivec2 cursorW = ivec2{ event.mouseX - w / 2, h / 2 - event.mouseY } - mCanvasOffset;
	mCanvasOffset -= ivec2(fac * vec2(cursorW));
}
void Canvas::process_event(const SDL_Event& event) {
	if (event.type == SDL_KEYDOWN) {
		process_key_down(event.key);
	}
	else if (event.type == SDL_KEYUP) {
		process_key_up(event.key);
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		process_mouse_button_down(event.button);
	}
	else if (event.type == SDL_MOUSEBUTTONUP) {
		process_mouse_button_up(event.button);
	}
	else if (event.type == SDL_MOUSEMOTION) {
		process_mouse_motion(event.motion);
	}
	else if (event.type == SDL_MOUSEWHEEL) {
		process_mouse_wheel(event.wheel);
	}
}
void Canvas::process_frame(float deltaTime) {}
void Canvas::render(ivec2 viewportSize) {
	GLuint image = mCanvasTexture;
	// Composite current stroke
	if (!mTools.empty()) {
		mTools.at(mCurTool)->composite(mCanvasDstTexture, mCanvasTexture);
		image = mCanvasDstTexture;
	}
	// 2x to account for the fact that OpenGL uses [-1, 1] not [0, 1]
	vec2 relativeOffset = 2 * vec2(mCanvasOffset) / vec2(viewportSize);
	float scale = powf(2.0f, mCanvasScaleLog);
	vec2 relativeScale = scale * vec2(mCanvasSize) / vec2(viewportSize);
	mat3 xform = {
		relativeScale.x, 0.0f, relativeOffset.x,
		0.0f, relativeScale.y, relativeOffset.y,
		0.0f, 0.0f, 1.0f
	};
	// Note: locations hardcoded in shader
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindVertexArray(mCanvasVAO);
	glUseProgram(mCanvasProgram);
	glUniformMatrix3fv(0, 1, GL_TRUE, xform.data());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image);
	glUniform1i(1, 0);
	glUniform4f(2, 1.0f, 1.0f, 1.0f, 1.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glUseProgram(0);

	// mCurTool should always be valid unless we have no tools at all
	if (!mTools.empty()) {
		auto [x, y] = cursor_window_coords();
		ivec2 screenMouse = { x, viewportSize.y - y };
		mTools.at(mCurTool)->preview(viewportSize, screenMouse, powf(2.0, mCanvasScaleLog));
	}
}
Canvas::SaveResponse Canvas::request_save() const {
	const int CANCEL = 0;
	const int DISCARD = 1;
	const int SAVE = 2;
	SDL_MessageBoxButtonData bData[3] = {
		{.flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, .buttonid=CANCEL, .text="Cancel"},
		{.flags = 0, .buttonid=DISCARD, .text="Discard"},
		{.flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, .buttonid=SAVE, .text="Save"},
	};
	SDL_MessageBoxData mData = {
		.flags = SDL_MESSAGEBOX_INFORMATION,
		.window = mWindow,
		.title = "Save",
		.message = "You have unsaved changes. Would you like to save them?",
		.numbuttons = 3,
		.buttons = bData,
		.colorScheme = NULL
	};
	int id = CANCEL;
	SDL_ShowMessageBox(&mData, &id);
	switch (id) {
	case CANCEL:
		return SaveResponse::CANCEL;
	case DISCARD:
		return SaveResponse::DISCARD;
	case SAVE:
		return SaveResponse::SAVE;
	default:
		std::abort();
	}
}
bool Canvas::prompt_new() {
	if (mModified) {
		auto response = request_save();
		if (response == SaveResponse::CANCEL)
			return false;
		if (response == SaveResponse::SAVE && !prompt_save())
			return false;
	}
	mShowNewDialog = true;
	return true;
}
bool Canvas::prompt_open() {
	if (mModified) {
		auto response = request_save();
		if (response == SaveResponse::CANCEL)
			return false;
		if (response == SaveResponse::SAVE && !prompt_save())
			return false;
	}
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
			set_canvas(canvasSize, pixels, path.get());
			return true;
		}
		else {
			fprintf(stderr, "[error] STBI error: %s", stbi_failure_reason());
		}
	}
	return false;
}
bool Canvas::prompt_save() {
	fprintf(stderr, "[info] dumping texture...");
	auto pixels = get_canvas();
	fprintf(stderr, " complete\n");

	nfdu8filteritem_t filters[1] = { { "PNG Images", "png" } };
	NFD::UniquePathU8 path = nullptr;
	auto res = NFD::SaveDialog(
		path,
		filters,
		1,
		mPath.empty() ? nullptr : mPath.parent_path().string().c_str(),
		mPath.empty() ? "output.png" : mPath.filename().string().c_str());

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
		mModified = false;
		return true;
	}
	return false;
}
void Canvas::run_tool_menu() {
	auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize;
	if (ImGui::Begin("Tool Panel", nullptr, windowFlags)) {
		for (size_t i = 0; i < mTools.size(); i++) {
			if (i % 4 > 0) ImGui::SameLine();
			if(ImGui::Selectable(mTools.at(i)->name(), i == mCurTool, 0, ImVec2(50, 50))) {
				if (i != mCurTool) {
					// user changed tools...
					set_current_tool(i);
				}
			}
		}
		ImGui::Text("Tool Configuration");
		if (!mTools.empty()) mTools.at(mCurTool)->run_ui();
	}
	ImGui::End();
}
void Canvas::run_main_menu() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "N", nullptr)) {
				prompt_new();
			}
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
ivec2 Canvas::cursor_window_coords() const {
	if (mInteractState == InteractState::CONFIGURE) {
		return mLastMousePos;
	}
	else {
		int x, y;
		SDL_GetMouseState(&x, &y);
		return ivec2{ x, y };
	}
}
vec2 Canvas::cursor_canvas_coords() const {
	float scale = powf(2.0f, mCanvasScaleLog);
	auto [x, y] = cursor_window_coords();
	int w, h;
	SDL_GetWindowSizeInPixels(mWindow, &w, &h);
	// cursor pos in window pixels relative to center of the image
	ivec2 cursorW = ivec2{ x - w / 2, h / 2 - y } - mCanvasOffset;
	// cursor pos in image pixels relative to the bottom left of the image
	return (vec2(cursorW) / scale) + (0.5 * vec2(mCanvasSize));
}
void Canvas::run_status_bar() {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();
	if (ImGui::BeginViewportSideBar("##StatusBar", NULL, ImGuiDir_Down, height, flags)) {
		if (ImGui::BeginMenuBar()) {
			ImGui::Text("Dimensions: %ix%i\t", mCanvasSize.x, mCanvasSize.y);
			ImGui::Text("Zoom: %g%%\t", powf(2.0f, mCanvasScaleLog) * 100);
			// ImGui::Text("Offset: %i,%i", mCanvasOffset.x, mCanvasOffset.y);
			vec2 cursor = cursor_canvas_coords();
			ImGui::Text("Cursor: (%5g,%5g)\t", cursor.x, cursor.y);
			ImGui::EndMenuBar();
		}
	}
	ImGui::End();
}
void Canvas::run_new_dialog() {
	constexpr size_t MAX_TEXTURE_DIMENSION = 8192;
	// I originally wanted to use a modal popup, but that would disable the main menu
	if (ImGui::Begin("New Canvas", &mShowNewDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputInt("Width", &mNewDialogCanvasSize.x, 16, 256);
		ImGui::InputInt("Height", &mNewDialogCanvasSize.y, 16, 256);
		if (mDidAStupid) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 31, 31, 255));
			ImGui::Text("Please enter a valid size.");
			ImGui::PopStyleColor();
		}
		if (ImGui::Button("Create")) {
			if (
				mNewDialogCanvasSize.x < 0 ||
				mNewDialogCanvasSize.x > MAX_CANVAS_AXIS ||
				mNewDialogCanvasSize.y < 0 ||
				mNewDialogCanvasSize.y > MAX_CANVAS_AXIS
			) {
				mDidAStupid = true;
			}
			else {
				mDidAStupid = false;
				// This automatically hides the new dialog
				set_canvas(mNewDialogCanvasSize, nullptr);
			}
		}
	}
	ImGui::End();
}
void Canvas::run_ui() {
	run_main_menu();
	if (mShowNewDialog) {
		run_new_dialog();
	} else {
		run_tool_menu();
	}
	run_status_bar();
}