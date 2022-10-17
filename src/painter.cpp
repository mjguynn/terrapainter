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
    GLfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
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

Painter::StrokeShader::StrokeShader() {
    mProgram = g_shaderMgr.compute("canvas_stroke");
    glUseProgram(mProgram);
    mSdfLocation = glGetUniformLocation(mProgram, "u_sdf");
    mV1Location = glGetUniformLocation(mProgram, "u_v1");
    mV2Location = glGetUniformLocation(mProgram, "u_v2");
    mF1Location = glGetUniformLocation(mProgram, "u_f1");
    glUseProgram(0);
}
void Painter::StrokeShader::submit(GLuint dest, ivec2 dims, int sdf, ivec2 v1, ivec2 v2, float f1) {
    glUseProgram(mProgram);
    glBindImageTexture(0, dest, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
    glUniform1i(mSdfLocation, sdf);
    glUniform2i(mV1Location, v1.x, v1.y);
    glUniform2i(mV2Location, v2.x, v2.y);
    glUniform1f(mF1Location, f1);
    glDispatchCompute(dims.x / 8, dims.y / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void Painter::StrokeShader::draw_circle(GLuint dest, ivec2 dims, ivec2 center, float radius) {
    submit(dest, dims, 0, center, ivec2::zero(), radius);
}
void Painter::StrokeShader::draw_rod(GLuint dest, ivec2 dims, ivec2 start, ivec2 end, float radius) {
    submit(dest, dims, 1, start, end, radius);
}

Painter::Painter(int width, int height)
    : mDims(width, height),
    mStrokeStart(std::nullopt),
    mRadius(20.0f),
    mRadiusMin(1.0f),
    mRadiusMax(100.0f),
    mColor(255, 0, 0),
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
    GLuint textures[2];
    glGenTextures(2, textures);
    mBaseT = textures[0];
    mStrokeT = textures[1];
    
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
    GLuint textures[2] = { mBaseT, mStrokeT };
    glDeleteTextures(2, textures);
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
        mStrokeStart = center;
        mStrokeS.draw_circle(mStrokeT, mDims, center, mRadius);
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        mStrokeStart = std::nullopt;
        //commit();
    }
    else if (mStrokeStart && event.type == SDL_MOUSEMOTION) {
        ivec2 current = { event.motion.x, mDims.y - event.motion.y };
        mStrokeS.draw_circle(mStrokeT, mDims, current, mRadius);
        mStrokeS.draw_rod(mStrokeT, mDims, mStrokeStart.value(), current, mRadius);
        mStrokeStart = current;
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

void Painter::commit() {
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mBufferT, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    
    glBindVertexArray(mVAO);
    {
        mCompositeS.use(mBaseT, mStrokeT, vec3(mColor) / 255.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::swap(mBufferT, mBaseT);
}
void Painter::draw() {
    constexpr auto TRANSFORM = mat4::identity();
    int x, y;
    SDL_GetMouseState(&x, &y);
    vec2 center = vec2{ x, mDims.y - y };

    // Select shader & draw
    glBindVertexArray(mVAO);
    {
        mCompositeS.use(mBaseT, mStrokeT, vec3(mColor) / 255.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        mCircleS.use(center, mRadius, 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);

}