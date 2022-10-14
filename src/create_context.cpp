#include "glad/glad.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include <iostream>
using namespace std;

// create vertices for triangle
float vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f, 0.5f, 0.0f};

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   " FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                   "}\0";

// -----------------Functions created for SDL ONLY SECTION (BEGIN)-------------
void set_pixel(
    SDL_Surface *const surface,
    const int x,
    const int y,
    const Uint8 r,
    const Uint8 g,
    const Uint8 b)
{
  Uint32 *const pixels = (Uint32 *)surface->pixels;
  const Uint32 color = SDL_MapRGB(surface->format, r, g, b);
  pixels[x + (y * surface->w)] = color;
}

int clamp(const int val, const int min, const int max)
{
  if (val < min)
    return min;
  if (val > max)
    return max;
  return val;
}
// -----------------Functions created for SDL ONLY SECTION (END)-------------

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

  // -------------SDL ONLY SECTION (BEGIN)---------------
  SDL_Surface *surface = SDL_GetWindowSurface(window);

  if (surface == NULL)
  {
    printf("surface is null");
    exit(1);
  }

  SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
  SDL_UpdateWindowSurface(window);

  srand(time(NULL));

  int mouseX = 0;
  int mouseY = 0;

  int x = 0;
  int y = 0;

  Uint8 r = 255;
  Uint8 g = 255;
  Uint8 b = 255;

  bool drawing = false;

  int strokeSize = 1;

  SDL_Event windowEvent;
  while (true)
  {
    if (SDL_PollEvent(&windowEvent))
    {
      if (windowEvent.type == SDL_QUIT)
        break;
      else if (windowEvent.type == SDL_MOUSEBUTTONDOWN)
      {
        drawing = true;
      }
      else if (windowEvent.type == SDL_MOUSEBUTTONUP)
      {
        drawing = false;
      }
      else if (windowEvent.type == SDL_MOUSEMOTION)
      {
        mouseX = windowEvent.motion.x;
        mouseY = windowEvent.motion.y;
      }
      else if (windowEvent.type == SDL_MOUSEWHEEL)
      {
        if (windowEvent.wheel.y > 0)
        {
          strokeSize = clamp((strokeSize + 1), 1, 50);
        }
        else if (windowEvent.wheel.y < 0)
        {
          strokeSize = clamp((strokeSize - 1), 1, 50);
        }
      }
      else if (windowEvent.type == SDL_KEYDOWN)
      {
        if (windowEvent.key.keysym.sym = SDLK_SPACE)
        {
          r = rand() % 255;
          g = rand() % 255;
          b = rand() % 255;
        }
      }
    }
    if (drawing)
    {
      // x = clamp(mouseX, 0, 800);
      // y = clamp(mouseY, 0, 600);
      for (int i = -strokeSize; i <= strokeSize; i++)
      {
        for (int j = -strokeSize; j <= strokeSize; j++)
        {
          if (sqrt(pow(i, 2) + pow(j, 2)) < strokeSize)
          {
            set_pixel(surface, clamp((mouseX + i), 0, 800), clamp((mouseY + j), 0, 800), r, g, b);
          }
        }
      }
      SDL_UpdateWindowSurface(window);
    }
  }

  SDL_DestroyWindow(window);

  // -------------SDL ONLY SECTION (END)---------------

  /*
  // Create OpenGL context
  SDL_GLContext context = SDL_GL_CreateContext(window);

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Set viewport
  glViewport(0, 0, 800, 600);

  // generate and bind VAO
  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // create VBO
  unsigned int VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // vertex shader
  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // fragment shader
  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // shader program
  unsigned int shaderProgram;
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // dealloc memory
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // set vertex attribute pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Run the event loop
  SDL_Event windowEvent;
  while (true)
  {
    // input

    // rendering commands
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

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
  */

  // Quit SDL
  SDL_Quit();
  return 0;
}
