#include "painter.h"
#include "imgui/imgui.h"

Painter::Painter(int width, int height)
    : mDims(width, height),
    mStrokeStart(std::nullopt),
    mRadius(20.0f),
    mRadiusMin(1.0f),
    mRadiusMax(100.0f),
    mColor(255, 0, 0),
    mPixels(size_t(width) * size_t(height)),
    mShader(),
    mUiCircleShader(width, height),
    mTexture(0), // Temp value
    mVAO(0), // Temp value
    mVBO(0), // Temp value
    mNeedsUpload(true)
{
    // Zero initialize
    mPixels.resize(width * height);

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

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
        int x, y;
        SDL_GetMouseState(&x, &y);
        ivec2 position = { x, mDims.y - y };
        mStrokeStart = position;
        draw_circle(position, mRadius, mColor);
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        mStrokeStart = std::nullopt;
    }
    else if (mStrokeStart && event.type == SDL_MOUSEMOTION) {
        ivec2 position = { event.motion.x, mDims.y - event.motion.y };
        draw_rod(mStrokeStart.value(), position, mRadius, mColor);
        mStrokeStart = position;
    }
    else if (event.type == SDL_MOUSEWHEEL) {
        auto scroll_delta = 4.0f * event.wheel.y;
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
// Find in-bounds pixels in a square region around `coord`
// Returns: (min_x, min_y) and (max_x, max_y)
std::pair<ivec2, ivec2> get_region_bounds(ivec2 dims, ivec2 coord, int radius) {
    int left = std::max(0, coord.x - radius);
    int right = std::min(dims.x, coord.x + radius);
    int bottom = std::max(0, coord.y - radius);
    int top = std::min(dims.y, coord.y + radius);
    return { ivec2{left, bottom}, ivec2 {right, top} };
}

void Painter::draw_circle(ivec2 position, float radius, RGBu8 color) {
    // left, right, bottom, top
    auto [min, max] = get_region_bounds(mDims, position, int(radius + 0.5));
    for (int i = min.x; i < max.x; i++) {
        for (int j = min.y; j < max.y; j++) {
            ivec2 coords = { i, j };
            vec2 offset(coords - position);
            if (offset.mag() <= radius) {
                set_pixel(coords, color);
            }
        }
    }
    mNeedsUpload = true;
}

void Painter::draw_rod(ivec2 start, ivec2 end, float radius, RGBu8 color) {
    ivec2 min, max;
    {
        int ir = static_cast<int>(radius + 0.5);
        auto bStart = get_region_bounds(mDims, start, ir);
        auto bEnd = get_region_bounds(mDims, end, ir);
        min = math::vmin(bStart.first, bEnd.first);
        max = math::vmax(bStart.second, bEnd.second);
    }

    ivec2 lineDir = end - start;

    for (int i = min.x; i < max.x; i++) {
        for (int j = min.y; j < max.y; j++) {
            ivec2 coord = { i, j };
            ivec2 relativeCoord = coord - start;
            float num = dot(lineDir, relativeCoord);
            float den = dot(lineDir, lineDir);
            float fac = num / den;

            vec2 offset;
            if (fac < 0) {
                offset = vec2(relativeCoord); // start is closest
            } else if (fac > 1) {
                offset = vec2(coord - end); // end is closest
            } else {
                offset = vec2(relativeCoord) - fac * vec2(lineDir);
            }

            if (offset.mag() <= radius) {
                set_pixel(coord, color);
            }
        }
    }
    mNeedsUpload = true;
}

void Painter::draw() {
    constexpr auto TRANSFORM = mat4::identity();
    int x, y;
    SDL_GetMouseState(&x, &y);
    vec2 center = vec2{ x, mDims.y - y };

    // Upload texture
    if (mNeedsUpload) {
        glBindTexture(GL_TEXTURE_2D, mTexture);
        glTexSubImage2D(
            GL_TEXTURE_2D, 
            0, 
            0, 
            0, 
            mDims.x, 
            mDims.y, 
            GL_RGB, 
            GL_UNSIGNED_BYTE, 
            mPixels.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);
        mNeedsUpload = false;
    }
    
    // Select shader & draw
    glBindVertexArray(mVAO);
    {
        mShader.use(TRANSFORM, mTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        mUiCircleShader.use(center, mRadius, 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);

}