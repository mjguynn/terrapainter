#include "painter.h"
#include "imgui/imgui.h"

Painter::Painter(int width, int height) 
	: mWidth(width), 
    mHeight(height), 
    mDrawing(false),
    mRadius(0.05),
    mPixels(width * height),
    mShader(),
    mTexture(0), // Temp value
    mVAO(0), // Temp value
    mVBO(0) // Temp value
{
    // Zero initialize
    mPixels.resize(width * height);

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    static float verts[] = {
         // POSITION                // TEXCOORD
         -1.0f,  -1.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         -1.0f,  +1.0f, 0.0f, 1.0f,   0.0f, 1.0f,
         +1.0f,  -1.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         +1.0f,  +1.0f, 0.0f, 1.0f,   1.0f, 1.0f,
    };

    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Painter::~Painter() {
    
}

void Painter::process_event(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        mDrawing = true;
        int x, y;
        SDL_GetMouseState(&x, &y);
        draw_circle(to_coords(x, y), mRadius);
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        mDrawing = false;
    }
    else if (event.type == SDL_MOUSEMOTION) {
        auto coords = to_coords(event.motion.x, event.motion.y);
        draw_circle(coords, mRadius);
    }
    else if (event.type == SDL_MOUSEWHEEL) {
        auto scroll_delta = 0.01f * event.wheel.y;
        mRadius = std::clamp(mRadius + scroll_delta, 0.01f, 1.0f);
    }
}

void Painter::process_ui() {
	TODO();
}

void Painter::draw_circle(vec2 coords, float radius) {
    // TODO();
}

void Painter::draw() {
    constexpr auto TRANSFORM = mat4::identity();

    // Upload texture
    glBindTexture(GL_TEXTURE_2D, mTexture);
    {
        glTexSubImage2D(
            GL_TEXTURE_2D, 
            0, 
            0, 
            0, 
            mWidth, 
            mHeight, 
            GL_RGB, 
            GL_UNSIGNED_BYTE, 
            mPixels.data()
        );
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Select shader & draw
    mShader.use(TRANSFORM, mTexture);
    glBindVertexArray(mVAO);
    {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);

   
}