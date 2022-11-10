#pragma once

#include "SDL.h"
#include "imgui/imgui.h"
#include "glad/gl.h"
#include "terrapainter/math.h"

// Abstract interface for canvas tools
// (i.e. brush, blur, texture splatter)
class ICanvasTool {
public:
	virtual ~ICanvasTool() noexcept = 0;
	// Returns the human-readable tool name.
	virtual const char* name() const = 0;
	// Clears relevant state to prepare for a fresh canvas texture.
	// canvasSize is guaranteed to be positive.
	virtual void clear(ivec2 canvasSize) = 0;
	// Creates or continues the current stroke. Strokes are ended by `reset`.
	// `modifier` indicates whether the modifier key (shift) is being held.
	virtual void stroke(ivec2 canvasMouse, bool modifier) = 0;
	// TODO: A bit complicated to explain
	// TODO: Leaking SDL details here is really ugly
	virtual void configure(SDL_KeyCode keyCode, ivec2 mouseDelta, bool modifier) = 0;
	// Composites the tool's output with the current Canvas content.
	virtual void composite(GLuint dst, GLuint src) = 0;
	// Draws/updates the IMGUI UI within an existing tool window.
	virtual void controls() = 0;
	// Render a fullscreen preview into the active framebuffer.
	// Prior to calling this, the alpha blending function is (TODO)
	// `screenSize` is guaranteed to be positive.
	virtual void preview(ivec2 screenSize, ivec2 screenMouse) = 0;
};

inline ICanvasTool::~ICanvasTool() noexcept {}