#pragma once

#include "core/math.hpp"
#include "core/vec2.hpp"
#include "core/mat3.hpp"
#include "core/color.hpp"

namespace inc::render {

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

    void init(const InitDesc& desc);
    void shutdown();

    void begin_frame(int window_width, int window_height);
    void end_frame();

    // ---- Primitives ------------------------------------------------------

    void draw_triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
    void draw_quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color);
    void draw_line(Vec2 a, Vec2 b, f32 thickness, Color color);
    void draw_circle(Vec2 center, f32 radius, Color color, int segments = 24);
    void draw_ring(Vec2 center, f32 r_inner, f32 r_outer,
                   Color color, int segments = 32);
    void draw_arc_ring(Vec2 center, f32 r_inner, f32 r_outer,
                       f32 a_start, f32 a_end,
                       Color color, int segments = 16);

private:
    void add_vertex(Vec2 p, Color color);

    void*  pipeline_      = nullptr;
    void*  vertex_buffer_ = nullptr;
    bool   initialized_   = false;

    struct Vtx { f32 x, y; f32 r, g, b, a; };
    static constexpr int MAX_VERTS = 65536;
    Vtx  cpu_verts_[MAX_VERTS];
    int  vert_count_ = 0;

    int  window_w_ = 1920;
    int  window_h_ = 1080;
};

} // namespace inc::render