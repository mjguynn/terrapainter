#include "painter.h"
#include "imgui/imgui.h"

Painter::Painter(int width, int height) 
	: mWidth(width), 
    mHeight(height), 
    mDrawing(false),
    mRadius(0.05),
    mPixels(0), // Temp value
    mTexture(0) // Temp value
{
    assert(width > 0 && height > 0);

    GLuint mPixels;
    glGenBuffers(1, &mPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPixels);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, mWidth * mHeight * sizeof(RGBu8), nullptr, GL_DYNAMIC_DRAW);
    void* pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);
    memset(pixels, 0, mWidth * mHeight * sizeof(RGBu8));
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        mWidth,
        mHeight,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        0 // offset into mPixels since mPixels is currently bound
    );

    // Avoid bugs, since having a pixel buffer bound
    // silently modifies other functions' behavior
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
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
    TODO();
}

void Painter::render() {
    TODO();
}