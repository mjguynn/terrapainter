#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <iostream>

int main(int argc, char *argv[])
{
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
  }

  // Set version
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  // Create window
  SDL_Window *window = SDL_CreateWindow("OpenGL", 100, 100, 800, 600, SDL_WINDOW_OPENGL);

  // Create OpenGL context
  SDL_GLContext context = SDL_GL_CreateContext(window);

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Set viewport
  glViewport(0, 0, 800, 600);

  // Run the event loop
  SDL_Event windowEvent;
  while (true)
  {
    // input

    // rendering commands
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // check and call events and swap the buffers
    if (SDL_PollEvent(&windowEvent))
    {
      if (windowEvent.type == SDL_QUIT)
        break;
      if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        glViewport(0, 0, windowEvent.window.data1, windowEvent.window.data2);
    }
    SDL_GL_SwapWindow(window);
  }

  // Destroy context
  SDL_GL_DeleteContext(context);
  // Quit SDL
  SDL_Quit();
  return 0;
}
