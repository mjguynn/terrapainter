#include "painter.h"
#include "imgui/imgui.h"

static float SCREENSPACE_QUAD[] = {
    // POSITION                // TEXCOORD
    -1.0f,  -1.0f, 0.0f, 1.0f,   0.0f, 0.0f,
    -1.0f,  +1.0f, 0.0f, 1.0f,   0.0f, 1.0f,
    +1.0f,  -1.0f, 0.0f, 1.0f,   1.0f, 0.0f,
    +1.0f,  +1.0f, 0.0f, 1.0f,   1.0f, 1.0f,
};

// Sets up & zero-initializes a canvas texture.
static void init_canvas(GLuint framebuffer, GLuint texture, GLenum internalFormat, GLenum format, int width, int height) {
    // Bind framebuffer, we'll be using it for initialization...
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

    // Setup canvas texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

    // Zero-initialize canvas texture
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    GLfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    // Reset texture & framebuffer
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

Painter::CompositeShader::CompositeShader() {
    mProgram = g_shaderMgr.graphics("canvas_composite");
    glUseProgram(mProgram);
    mBlendModeLocation = glGetUniformLocation(mProgram, "u_blendMode");
    mBaseTextureLocation = glGetUniformLocation(mProgram, "u_baseTexture");
    mLayerMaskLocation = glGetUniformLocation(mProgram, "u_layerMask");
    mLayerTintLocation = glGetUniformLocation(mProgram, "u_layerTint");
    glUseProgram(0);
}
void Painter::CompositeShader::use(GLuint baseT, GLuint layerT, vec3 layerTint) {
    glUseProgram(mProgram);
    glUniform1i(mBlendModeLocation, 0);
    glUniform1i(mBaseTextureLocation, 0);
    glUniform1i(mLayerMaskLocation, 1);
    glUniform3f(mLayerTintLocation, layerTint.x, layerTint.y, layerTint.z);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseT);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, layerT);
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

Painter::StrokeShader::StrokeShader() {
    mProgram = g_shaderMgr.compute("canvas_stroke");
    glUseProgram(mProgram);
    mSdfLocation = glGetUniformLocation(mProgram, "u_sdf");
    mOriginLocation = glGetUniformLocation(mProgram, "u_origin");
    mV1Location = glGetUniformLocation(mProgram, "u_v1");
    mV2Location = glGetUniformLocation(mProgram, "u_v2");
    mF1Location = glGetUniformLocation(mProgram, "u_f1");
    mF2Location = glGetUniformLocation(mProgram, "u_f2");
    glUseProgram(0);
}
void Painter::StrokeShader::submit(GLuint dest, ivec2 origin, ivec2 size, int sdf, ivec2 v1, ivec2 v2, float f1, float f2) {
    glUseProgram(mProgram);
    glBindImageTexture(0, dest, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
    glUniform1i(mSdfLocation, sdf);
    glUniform2i(mOriginLocation, origin.x, origin.y);
    glUniform2i(mV1Location, v1.x, v1.y);
    glUniform2i(mV2Location, v2.x, v2.y);
    glUniform1f(mF1Location, f1);
    glUniform1f(mF2Location, f2);
    glDispatchCompute( (size.x + 7) / 8, (size.y + 7) / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void Painter::StrokeShader::draw_circle(GLuint dest, ivec2 dims, ivec2 center, float radius) {
    auto [min, max] = get_region_bounds(dims, center, radius);
    submit(dest, min, max-min, 0, center, ivec2::zero(), radius, 0);
}
void Painter::StrokeShader::draw_rod(GLuint dest, ivec2 dims, ivec2 startPos, ivec2 endPos, float startRadius, float endRadius) {
    ivec2 min, max;
    {
        auto bStart = get_region_bounds(dims, startPos, int(startRadius + 0.5));
        auto bEnd = get_region_bounds(dims, endPos, int(endRadius + 0.5));
        min = math::vmin(bStart.first, bEnd.first);
        max = math::vmax(bStart.second, bEnd.second);
    }
    submit(dest, min, max - min, 1, startPos, endPos, startRadius, endRadius);
}

Painter::Painter(int width, int height)
    : mDims(width, height),
    mStrokeState(std::nullopt),
    mRadius(20.0f),
    mRadiusMin(1.0f),
    mRadiusMax(100.0f),
    mColor(1.0f, 1.0f, 1.0f),
    mCompositeS(),
    mStrokeS(), // TODO
    mCircleS(width, height),
    mBaseT(0), // Temp value
    mBufferT(0), // Temp value
    mStrokeT(0), // Temp value
    mFramebuffer(0), // Temp value
    mVAO(0), // Temp value
    mVBO(0) // Temp value
{
    // Name framebuffer & textures
    glGenFramebuffers(1, &mFramebuffer);
    GLuint textures[3];
    glGenTextures(3, textures);
    mBaseT = textures[0];
    mBufferT = textures[1];
    mStrokeT = textures[2];
    
    init_canvas(mFramebuffer, mBaseT, GL_RGB8, GL_RGB, width, height);
    init_canvas(mFramebuffer, mBufferT, GL_RGB8, GL_RGB, width, height);
    init_canvas(mFramebuffer, mStrokeT, GL_R8, GL_RED, width, height);

    // TODO: Refactor this with a more general mesh class
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SCREENSPACE_QUAD), SCREENSPACE_QUAD, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Painter::~Painter() {
    GLuint textures[3] = { mBaseT, mBufferT, mStrokeT };
    glDeleteTextures(3, textures);
    glDeleteFramebuffers(1, &mFramebuffer);
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
}

void Painter::process_event(SDL_Event& event, ImGuiIO& io) {
    if (io.WantCaptureMouse) {
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        ivec2 center = { x, mDims.y - y };
        mStrokeState = StrokeState{ .last_position = center, .last_radius = mRadius };
        mStrokeS.draw_circle(mStrokeT, mDims, center, mRadius);
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        mStrokeState = std::nullopt;
        commit();
    }
    else if (mStrokeState.has_value() && event.type == SDL_MOUSEMOTION) {
        ivec2 current = { event.motion.x, mDims.y - event.motion.y };
        mStrokeS.draw_circle(mStrokeT, mDims, current, mRadius);
        mStrokeS.draw_rod(mStrokeT, 
            mDims, 
            mStrokeState->last_position, 
            current, 
            mStrokeState->last_radius, 
            mRadius
        );
        mStrokeState->last_position = current;
        mStrokeState->last_radius = mRadius;
    }
    else if (event.type == SDL_MOUSEWHEEL) {
        auto scroll_delta = 4.0f * event.wheel.y;
        mRadius = std::clamp(mRadius + scroll_delta, mRadiusMin, mRadiusMax);
    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT) {
        mRadius += 10;
    }
}

void Painter::draw_ui() {
    if (ImGui::Begin("Paint Controls", nullptr, 0)) {
        float height = mColor.x;
        ImGui::SliderFloat("Height", &height, 0.0f, 1.0f, "%.3f");
        mColor = vec3::splat(height);
        ImGui::SliderFloat("Brush Radius", &mRadius, mRadiusMin, mRadiusMax, "%.2f", 0);
    }
    ImGui::End();
}

void Painter::commit() {
    GLfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    // commit stroke to base & swap buffers
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBufferT, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glBindVertexArray(mVAO);
    {
        mCompositeS.use(mBaseT, mStrokeT, mColor);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);
    std::swap(mBaseT, mBufferT);

    // clear stroke layer
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mStrokeT, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearBufferfv(GL_COLOR, 0, clearColor);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Painter::draw() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    vec2 center = vec2{ x, mDims.y - y };

    // Select shader & draw
    glBindVertexArray(mVAO);
    {
        mCompositeS.use(mBaseT, mStrokeT, mColor);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        mCircleS.use(center, mRadius, 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);

}