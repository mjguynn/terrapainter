#pragma once

#include <SDL.h>

// The reason this is moved outside of main is to enforce proper cleanup order
// Right now we have a bunch of objects with destructors that do OpenGL API calls...
// but if we stick them in main, those destructors will run after we've already
// closed the OpenGL context!
// ... also it's nice to seperate boilerplate from app logic
void run_terrapainter(SDL_Window* window);