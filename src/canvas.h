#pragma once

// Canvas2 Checklist: (aka Pure Condensed Scope Creep)
//	𐌢 Cleanup existing code... use less backbuffers, unnecessary syncs, etc
//  𐌢 Abstract from SDL, maybe? 
//		|-> Is this a good idea?
//  𐌢 Quadratic stroke interpolation (lookahead one frame)
//		|-> Involves solving a quadratic equation on the GPU, should be ok
//		|-> Is adding an extra frame of lag unacceptable?
//		|-> (Could always switch to software cursor or hide cursor while drawing)
//  𐌢 Separate from image size
//		|-> Use scrollwheel to zoom in/out, shift-mclick to pan
//	𐌢 Extend paintbrush
//		|-> Parametrize by outer radius (at 0) inner radius (at 1) & hardness (degree of polynomial)
//		|-> Note that all these can be implemented on layers on top of the fundamental SDF
//	𐌢 Accumulate vs Continuous mode
//		|-> continuous mode does "perfect interpolation" along stroke, doesn't commit stroke until mouse up
//		|-> accumulate mode does the basic "spit out a bunch of circles in a loop"
//	𐌢 Allow brush textures
//		|-> Will force accumulate mode, unsure how one would do perfect interpolation with a brush texture
//		|-> For loop in the compute shader? Cache texture reads within workgroup?
//	𐌢 Blur brush
//		|-> GPU convolution
//		|-> For radius: instead of lerping with unblurred background, as is the norm, lerp the KERNELS
//		|-> This way we get a kinda smooth falloff
//  𐌢 Change cursor to "hitmarker" when not over UI element?
//  𐌢 Somehow dock brush stroke UI to the side of the screen
//  𐌢 More keyboard shortcuts!!!
//  𐌢 Undo/Redo

// ==== Considerations for undo/redo =====
// A 1920x1080 texture w/ 8bit RGBA is 8MB
// The Iceland heightmap is 18MB in GPU memory
// 
// To keep memory pressure light, we don't want
// to keep the undo textures on the GPU...
// Instead we want to transfer them to main memory
// This is a really slow operation so we should do it
// async with a pixel buffer object
// 
// This requires keeping the old texture valid so we shouldn't
// final-composite onto the current mCanvasTexture, we should 
// copy it and shove the old one in a command object along with
// a sync object, create a PBO and set up the transfer
// 
// However if the user undoes *before* the transfer completes,
// we should swap the current mCanvasTexture with the previous texture
// Do we delete the mCanvasTexture we just discarded? No -- we've
// gotta repeat the same process except now we stick it in the redo
// stack.
//
// Again, to keep memory light, we should somehow record which ranges
// of the image were modified. GIMP uses a tile-based system where they
// simply copy out hit tiles. We don't need to be that advanced, we
// can probably get away with minX/minY and maxX/maxY since we already
// compute that for the brush compute shader
//
// If this *actually* causes issues in practice we should move to tiles
// since there are a ton of pathological cases for minmax
// 
// To destroy the associated GPU resources we'll ideally check status of
// sync objects each frame

#include <memory>
#include <vector>

#include <glad/gl.h>

#include "terrapainter/math.h"
#include "terrapainter.h"

struct CanvasRegion {
	ivec2 min; // Min X & Y coordinates of the region
	ivec2 max; // Max X & Y coordinates of the region
};

// Abstract interface for canvas tools
// (i.e. brush, blur, texture splatter)
class ICanvasTool {
public:
	virtual ~ICanvasTool() noexcept = 0;
	// Returns the human-readable tool name.
	virtual const char* name() const = 0;
	// Clears stroke state to prepare for a fresh canvas texture.
	// canvasSize is guaranteed to be positive.
	virtual void clear_stroke(ivec2 canvasSize) = 0;
	// Creates or continues the current stroke. Strokes are ended by `reset`.
	// `modifier` indicates whether the modifier key (shift) is being held.
	virtual void update_stroke(ivec2 canvasMouse, bool modifier) = 0;
	// TODO: A bit complicated to explain
	// TODO: Leaking SDL details here is really ugly
	virtual void update_param(SDL_KeyCode keyCode, ivec2 mouseDelta, bool modifier) = 0;
	// Composites the tool's output with the current Canvas content.
	// Returns a maximal bound on the modified region.
	virtual CanvasRegion composite(GLuint dst, GLuint src) = 0;
	// Draws/updates the IMGUI UI within an existing tool window.
	virtual void run_ui() = 0;
	// Render a fullscreen preview into the active framebuffer.
	// Prior to calling this, the alpha blending function is GL_ONE_MINUS_SRC_ALPHA
	// and should remain that way afterwards
	// `screenSize` is guaranteed to be positive.
	virtual void preview(ivec2 screenSize, ivec2 screenMouse) = 0;
};

inline ICanvasTool::~ICanvasTool() noexcept {}

class Canvas : public virtual IApp {
public:
	using ToolIndex = size_t;
private:
	// The set of all registered tools
	std::vector<std::unique_ptr<ICanvasTool>> mTools;
	// The index of the current tool within mTools
	ToolIndex mCurTool;

	// The pixel offset relative to the *center* of the canvas,
	// post-scale.
	ivec2 mCanvasOffset;
	// The scale of the canvas
	// This is an absolute scale, it doesn't change with mDims
	float mCanvasScale;

	// The dimensions of the canvas texture(s)
	// Invariant: This is kept in sync with mCanvasTexture
	ivec2 mCanvasSize;
	// Handle to the current canvas texture
	GLuint mCanvasTexture;
	// Handle to the canvas swap texture
	// This is used as the "write target" while compositing
	// a non-final stroke, once the stroke is finalized this becomes
	// the main canvas texture and the canvas texture is cleared
	GLuint mCanvasSwapTexture;

	SDL_Window* mWindow;
	// True if there are changes which haven't been saved to disk
	bool mModified;

	enum class SaveResponse {
		SAVE,
		DISCARD,
		CANCEL
	};
	SaveResponse request_save() const;

public:
	Canvas(SDL_Window* window);
	~Canvas() noexcept override;

	Canvas(const Canvas&) = delete;
	Canvas& operator=(const Canvas&) = delete;

	Canvas(Canvas&&) = delete;
	Canvas& operator=(Canvas&&) = delete;

	ivec2 get_canvas_size() const;

	// Returns the pixels comprising the canvas (RGBA)
	std::vector<uint8_t> get_canvas() const;
	// Sets the pixels comprising the canvas (RGBA)
	void set_canvas(ivec2 canvasSize, uint8_t* pixels);

	bool prompt_open();
	bool prompt_save();

	// Registers the given tool and returns its tool index.
	ToolIndex register_tool(std::unique_ptr<ICanvasTool> tool);

	// Sets the current tool. This acts as if the user
	// manually switched tools (so it will commit strokes,
	// do any other side effects, etc)
	void set_current_tool(ToolIndex toolIndex);

	// IApp implementation
	void activate() override;
	void deactivate() override;
	void process_event(const SDL_Event& event) override;
	void process_frame(float deltaTime) override;
	void render(ivec2 viewportSize) const override;
	void run_ui() override;
};