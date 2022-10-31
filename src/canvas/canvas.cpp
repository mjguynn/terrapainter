#include "canvas.h"

Canvas::~Canvas() noexcept {
	// Delete canvas texture (if it exists)
	if (mCanvasTexture) glDeleteTextures(1, &mCanvasTexture);
}

Canvas::Canvas(Canvas&& moved) noexcept {
	*this = std::move(moved);
}

Canvas& Canvas::operator=(Canvas&& moved) noexcept {
	mTools = std::move(moved.mTools);
	mCurTool = moved.mCurTool;
	mCanvasPos = moved.mCanvasPos;
	mCanvasScale = moved.mCanvasScale;
	mCanvasTexture = moved.mCanvasTexture;
	moved.mCanvasTexture = 0;
}

Canvas::ToolIndex Canvas::register_tool(std::unique_ptr<ICanvasTool> tool) {
	mTools.emplace_back(std::move(tool));
	return mTools.size() - 1;
}

void Canvas::set_current_tool(size_t toolIndex) {
	assert(toolIndex < mTools.size());
	mCurTool = toolIndex;
}