#include "canvas.h"
Canvas::Canvas(ivec2 size) {
	set_viewport_size(size);
	mTools = {};
	mCurTool = 0;
	mCanvasPos = vec2(size) / 2.0f;
	mCanvasScale = 1.0f;
	mCanvasSize = ivec2::zero();
	mCanvasTexture = 0;
	mCanvasSwapTexture = 0;
}
Canvas::~Canvas() noexcept {
	if (mCanvasTexture) glDeleteTextures(1, &mCanvasTexture);
	if (mCanvasSwapTexture) glDeleteTextures(1, &mCanvasSwapTexture);
}
Canvas::Canvas(Canvas&& moved) noexcept {
	*this = std::move(moved);
}
Canvas& Canvas::operator=(Canvas&& moved) noexcept {
	mViewportSize = moved.mViewportSize;
	mTools = std::move(moved.mTools);
	mCurTool = moved.mCurTool;
	mCanvasPos = moved.mCanvasPos;
	mCanvasScale = moved.mCanvasScale;
	mCanvasSize = moved.mCanvasSize;
	mCanvasTexture = moved.mCanvasTexture;
	moved.mCanvasTexture = 0;
	mCanvasSwapTexture = moved.mCanvasSwapTexture;
	moved.mCanvasSwapTexture = 0;
}
ivec2 Canvas::viewport_size() const {
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