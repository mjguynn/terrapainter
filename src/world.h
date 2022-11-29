#pragma once

#include "terrapainter.h"
#include "canvas.h"
#include "terrapainter/scene/camera.h"
#include "terrapainter/scene/entity.h"
#include "scene/controllers.h"
#include "scene/terrain.h"

// This is kind of a misnomer, as its conflating the "world" (in a scene 
// graph sense) with a bunch of app logic. This is the cause of the ICKY 
// multiple inheritance. It's cleaner to write a "Explorer" class for
// the app logic and then make "World" strictly a scenegraph node
// which is a member of the Explorer class.
class World : public Entity, public virtual IApp {
	// The canvas the data is sourced from
	Canvas& mSource;

	// The actual terrain
	Terrain* mTerrain;

	// The camera to draw the scene from
	Camera* mActiveCamera;

	// Controls the active camera.
	NoclipController mCameraController;

	// Whether to show the debug camera window.
	bool mShowCameraControls;

	ivec2 mLastViewportSize;
	GLuint mReflectionFramebuffer;
	GLuint mReflectionTexture;
	GLuint mReflectionDepth;

	void run_camera_control_ui();

public:
	World(Canvas& source);
	~World() noexcept override;

	// Returns texture for water reflection around z = 0
	// Kinda nasty that this is hardcoded, but this is due on Wednesday...
	GLuint reflection_texture();

	// IApp implementation
	// Remind me again why the hell I have to rewrite
	// EACH and EVERY member declaration when I inherit?
	void activate() override;
	void deactivate() override;
	void process_event(const SDL_Event& event) override;
	void process_frame(float deltaTime) override;
	void render(ivec2 viewportSize) override;
	void run_ui() override;
};