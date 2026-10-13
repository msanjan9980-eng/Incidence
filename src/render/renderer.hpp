#pragma once

#include "core/math.hpp"
#include "core/mat3.hpp"

namespace inc::render {

// ---------------------------------------------------------------------------
// Renderer — thin wrapper over sokol_gfx.
//
// Owns: device init, the triangle pipeline, the vertex buffer, and the
// per-frame draw submission. Higher layers (App, game entities) never touch
// sokol_gfx directly.
//
// M2 scope: one pipeline, one vertex buffer, one draw call. The same class
// will grow to hold many pipelines, offscreen targets, and the post pass in
// later milestones.
// ---------------------------------------------------------------------------

struct InitDesc {
    int window_width  = 1920;
    int window_height = 1080;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Initialize sokol_gfx. Call once, after the window is created.
    void init(const InitDesc& desc);

    // Release all GPU resources. Call once, before the window closes.
    void shutdown();

    // Begin a frame: set up viewport + clear. Called at the top of each frame.
    void begin_frame(int window_width, int window_height);

    // Draw one instance of the triangle at the given transform with the
    // given colour. Called from App::render for each entity.
    void draw_triangle(const Mat3& transform,
                       f32 r, f32 g, f32 b, f32 a);

    // End a frame: submit all queued work. Called at the bottom of each frame.
    void end_frame();

private:
    // Handles to GPU resources. All are sokol_gfx opaque IDs; we keep them
    // as void* here to avoid leaking sokol headers into this file.
    void*  pipeline_     = nullptr;
    void*  vertex_buffer_= nullptr;
    bool   initialized_  = false;
};

} // namespace inc::render