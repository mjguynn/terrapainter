#pragma once

// Canvas2 Checklist: (aka Pure Condensed Scope Creep)
//	✔️ Cleanup existing code... use less backbuffers, unnecessary syncs, etc
//  ✔️ Abstract from SDL, maybe? 
//		|-> Is this a good idea?
//  𐌢 Quadratic stroke interpolation (lookahead one frame)
//		|-> Involves solving a quadratic equation on the GPU, should be ok
//		|-> Is adding an extra frame of lag unacceptable?
//		|-> (Could always switch to software cursor or hide cursor while drawing)
//  ✔️ Separate from image size
//		|-> Use scrollwheel to zoom in/out, right click & drag to pan
//	𐌢 Extend paintbrush
//		|-> Parametrize by outer radius (at 0) & hardness (degree of polynomial)
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
//  ✔️ ~~Somehow dock brush stroke UI to the side of the screen~~ nah
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
#include <filesystem>

#include <glad/gl.h>

#include "terrapainter/math.h"
#include "terrapainter.h"

// The maximum supported size of the axis of a Canvas texture.
constexpr size_t MAX_CANVAS_AXIS = 8192;

struct CanvasRegion {
	ivec2 min; // Min X & Y coordinates of the region
	ivec2 max; // Max X & Y coordinates of the region

	CanvasRegion(ivec2 min, ivec2 max) : min(min), max(max) {}
	CanvasRegion(vec2 point, float radius) {
		min = ivec2(point - vec2::splat(radius));
		max = ivec2(point + vec2::splat(0.5 + radius));
	}
	static CanvasRegion merge(const CanvasRegion& a, const CanvasRegion& b) {
		return CanvasRegion(math::vmin(a.min, b.min), math::vmax(a.max, b.max));
	}
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
	virtual void update_stroke(vec2 canvasMouse, bool modifier) = 0;
	// TODO: explain
	virtual bool understands_param(SDL_Keycode keyCode) = 0;
	// TODO: A bit complicated to explain
	// TODO: Leaking SDL details here is really ugly
	virtual void update_param(SDL_Keycode keyCode, ivec2 mouseDelta, bool modifier) = 0;
	// Composites the tool's output with the current Canvas content.
	// Returns a maximal bound on the modified region.
	virtual void composite(GLuint dst, GLuint src) = 0;
	// Draws/updates the IMGUI UI within an existing tool window.
	virtual void run_ui() = 0;
	// Render a fullscreen preview into the active framebuffer.
	// Prior to calling this, the alpha blending function is GL_ONE_MINUS_SRC_ALPHA
	// and should remain that way afterwards
	// `screenSize` is guaranteed to be positive.
	virtual void preview(ivec2 screenSize, ivec2 screenMouse, float canvasScale) = 0;
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

	// The pixel offset relative to the *center* of the canvas.
	ivec2 mCanvasOffset;
	// The logarithmic scale of the canvas (log2)
	float mCanvasScaleLog;

	// The dimensions of the canvas texture(s)
	// Invariant: This is kept in sync with mCanvasTexture
	ivec2 mCanvasSize;
	// Handle to the current canvas texture
	GLuint mCanvasTexture;
	// Handle to the canvas destination texture
	// This is used as the "write target" while compositing
	// a non-final stroke, once the stroke is finalized this becomes
	// the main canvas texture and the canvas texture is cleared
	GLuint mCanvasDstTexture;

	// Handle to the program used for drawing the canvas onscreen
	// This is pretty basic, pretty much just a blit
	GLuint mCanvasProgram;

	// VAO and VBO for the quad used for drawing the canvas
	GLuint mCanvasVAO;
	GLuint mCanvasVBO;

	// This is used with mInteractState for configuring the tools
	SDL_Keycode mHeldKey;
	// This is used for save/restore mouse position in relative mode,
	// it shouldn't be necessary except SDL2 is BUGGY and literally does not work
	// This is in SDL coordinates btw
	ivec2 mLastMousePos;

	SDL_Window* mWindow;
	// True if there are changes which haven't been saved to disk
	bool mModified;
	// Path to the currently opened file. Empty if file didn't come from disk.
	std::filesystem::path mPath;
	// Whether the new dialog is open, this is modal so we block out everything else but the main menu
	bool mShowNewDialog; 
	ivec2 mNewDialogCanvasSize; // the size used in the new file dialog
	bool mDidAStupid; // if the user "accidentally" entered an invalid size in the new file dialog

	enum class InteractState {
		NONE,
		PAN,
		STROKE,
		CONFIGURE, // configuring tool
		// SWITCH, <-- tool quickswitch, DOOM/UT4 style... I was planning on doing this but probably no time...
	} mInteractState;

	enum class SaveResponse {
		SAVE,
		DISCARD,
		CANCEL
	} request_save() const;

	// Splitting up functions for my own sanity
	void process_key_down(const SDL_KeyboardEvent& event);
	void process_key_up(const SDL_KeyboardEvent& event);
	void process_mouse_button_down(const SDL_MouseButtonEvent& event);
	void process_mouse_button_up(const SDL_MouseButtonEvent& event);
	void process_mouse_motion(const SDL_MouseMotionEvent& event);
	void process_mouse_wheel(const SDL_MouseWheelEvent& event);

	void run_tool_menu();
	void run_main_menu();
	void run_status_bar();
	void run_new_dialog();

	// Returns the position of the cursor in window coords
	// This is its own function because we need to fake a static cursor during interactions
	// because SDL SUCKS!!!
	ivec2 cursor_window_coords() const;

	// Returns the position of the cursor in canvas coordinates
	// That is, affected by scale, and relative to the canvas origin (bottom left)
	vec2 cursor_canvas_coords() const;

	void set_interact_state(InteractState s);
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
	// If pixels is nullptr, then it will create a blank texture of the requested size
	// Source is used to track where this canvas came from
	// Returns false if the size is invalid
	bool set_canvas(ivec2 canvasSize, uint8_t* pixels, std::filesystem::path source = std::filesystem::path());

	bool prompt_new();
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