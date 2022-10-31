#include "SDL.h"
#include "imgui/imgui.h"
#include "glad/gl.h"

// Abstract interface for canvas tools
// (i.e. brush, texture splatter)
class ICanvasTool {
public:
	// Returns the human-readable tool name.
	virtual const char* name() = 0;
	// Called whenever a relevant SDL event is issued.
	virtual void process_event(SDL_Event& e) = 0;
	// Called once every frame.
	virtual void process_frame(float deltaTime) = 0;
	// Composites the tool's output 
	virtual void composite(GLuint dst, GLuint src) = 0;
	virtual ~ICanvasTool() = 0;
};


inline ICanvasTool::~ICanvasTool() {}