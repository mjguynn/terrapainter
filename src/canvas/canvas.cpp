#include "canvas.h"
// TODO: Move this to a common header or something, it's pretty useful
static void configure_texture(GLuint texture, GLenum min, GLenum mag, GLenum sWrap, GLenum tWrap) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
}
Canvas::Canvas(ivec2 viewportSize) {
	set_viewport_size(viewportSize);
	mTools = std::vector<std::unique_ptr<ICanvasTool>>();
	mCurTool = 0;
	mCanvasPos = vec2(viewportSize) / 2.0f;
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
ivec2 Canvas::get_viewport_size() const {
	return mViewportSize;
}
void Canvas::set_viewport_size(ivec2 size) {
	assert(size.x > 0 && size.y > 0);
	mViewportSize = size;
}
Canvas::ToolIndex Canvas::register_tool(std::unique_ptr<ICanvasTool> tool) {
	mTools.emplace_back(std::move(tool));
	return mTools.size() - 1;
}
void Canvas::set_current_tool(ToolIndex toolIndex) {
	assert(toolIndex < mTools.size());
	mCurTool = toolIndex;
}