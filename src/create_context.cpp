#include <cmath>
#include "glad/glad.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "stb/stb_image.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "terrapainter/shader_s.h"
#include "terrapainter/util.h"

int main(int argc, char *argv[])
{

    // Initialize SDL
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        error("Failed to initialize SDL: %s\n", SDL_GetError());
    }

    // Set version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window *window = SDL_CreateWindow("Terrapainter", 100, 100, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    // Create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Set up IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();
  
    int claimed_width, claimed_height;
    SDL_GetWindowSize(window, &claimed_width, &claimed_height);
    int real_width, real_height;
    SDL_GL_GetDrawableSize(window, &real_width, &real_height);

    float dpi_scale = float(real_width * real_height) / float(claimed_width * claimed_height);
    io.Fonts->AddFontFromFileTTF("../cs4621/extern/source-code-pro/SourceCodePro-Regular.ttf", floor(16 * dpi_scale));

    style.ScaleAllSizes(1.0f / dpi_scale);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        error("Failed to initialize GLAD");
    }

  // Set viewport
  glViewport(0, 0, 800, 600);

  Shader shader("../cs4621/shaders/3.3.shader.vs", "../cs4621/shaders/3.3.shader.fs");
  
  float vertices[] = {
    // positions          // colors           // texture coords
    0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
    0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
  };

  unsigned int indices[] = {
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
  };  

  // create Vertex Array Object, Vertex Buffer Object, and Element Buffer Object
  unsigned int VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // set vertex attribute pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Textures 
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // set the texture wrapping/filtering options (on the currently bound texture_2D object)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load and generate the texture
  int width, height, nrChannels;
  unsigned char *data = stbi_load("../cs4621/images/container.jpg", &width, &height, &nrChannels, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);
  shader.use();
  shader.setInt("texture1", 0);
 

  bool running = true;
  bool show_demo_window = true;

  // Run the event loop
  SDL_Event windowEvent;
  while (running)
  {
    // input

    // rendering commands
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Shaders
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader.use();

    // render the triangles
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // handle events
    while (SDL_PollEvent(&windowEvent))
    {
      ImGui_ImplSDL2_ProcessEvent(&windowEvent);
      // This makes dragging windows feel snappy
      io.MouseDrawCursor = ImGui::IsMouseDragging(0);

      if (windowEvent.type == SDL_QUIT) {
          running = false;
      }
      // This ridiculous if statement is from https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl3/main.cpp
      else if (
          windowEvent.type == SDL_WINDOWEVENT && 
          windowEvent.window.event == SDL_WINDOWEVENT_CLOSE && 
          windowEvent.window.windowID == SDL_GetWindowID(window)
      ) {
          running = false;
      }
      else if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          glViewport(0, 0, windowEvent.window.data1, windowEvent.window.data2);
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap buffers
    SDL_GL_SwapWindow(window);
  }

  // Shutdown IMGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  // Shutdown SDL
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
