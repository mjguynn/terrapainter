#include "painter.h"
#include "imgui/imgui.h"

Painter::Painter(int width, int height) 
	: mWidth(width), 
    mHeight(height), 
    mDrawing(false),
    mRadius(20.0f),
    mRadiusMin(1.0f),
    mRadiusMax(100.0f),
    mColor(255, 0, 0),
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

void Painter::process_event(SDL_Event& event, ImGuiIO& io) {
    if (io.WantCaptureMouse) {
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        mDrawing = true;
        int x, y;
        SDL_GetMouseState(&x, &y);
        draw_circle(x, mHeight - y, mRadius, mColor);
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        mDrawing = false;
    }
    else if (mDrawing && event.type == SDL_MOUSEMOTION) {
        draw_circle(event.motion.x, mHeight - event.motion.y, mRadius, mColor);
    }
    else if (event.type == SDL_MOUSEWHEEL) {
        auto scroll_delta = 0.5f * event.wheel.y;
        mRadius = std::clamp(mRadius + scroll_delta, mRadiusMin, mRadiusMax);
    }
}

void Painter::draw_ui() {
    vec3 rgb = static_cast<vec3>(mColor) / 255.0f;
    if (ImGui::Begin("Paint Controls", nullptr, 0)) {
        ImGui::ColorPicker3("Brush Color", reinterpret_cast<float*>(&rgb), 0);
        mColor = static_cast<RGBu8>(rgb * 255.0f);
        ImGui::SliderFloat("Brush Radius", &mRadius, mRadiusMin, mRadiusMax, "%.2f", 0);
    }
    ImGui::End();
}

void Painter::draw_circle(int x, int y, float radius, RGBu8 color) {
    int iRadius = int(radius);
    int left = std::max(0, x-iRadius);
    int right = std::min(mWidth, x + iRadius);
    int bottom = std::max(0, y - iRadius);
    int top = std::min(mHeight, y + iRadius);

    for (int i = left; i < right; i++) {
        for (int j = bottom; j < top; j++) {
            vec2 offset = { float(i-x), float(j-y) };
            if (offset.mag() <= mRadius) {
                set_pixel(i, j, color);
            }
        }
    }
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