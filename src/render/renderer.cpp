#include "render/renderer.hpp"
#include "render/shaders.hpp"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#include <cstdio>

namespace inc::render {

namespace {

inline void unpack_color(Color c, f32& out_r, f32& out_g, f32& out_b, f32& out_a) {
    constexpr f32 inv255 = 1.0f / 255.0f;
    out_r = static_cast<f32>(color_r(c)) * inv255;
    out_g = static_cast<f32>(color_g(c)) * inv255;
    out_b = static_cast<f32>(color_b(c)) * inv255;
    out_a = static_cast<f32>(color_a(c)) * inv255;
}

// Build the projection matrix that maps virtual pixels (0,0 at top-left)
// into NDC (-1..1), letterboxed to fit the current window.
Mat3 build_projection(int window_w, int window_h) {
    const f32 vw = VIRTUAL_W;
    const f32 vh = VIRTUAL_H;
    const f32 ww = static_cast<f32>(window_w);
    const f32 wh = static_cast<f32>(window_h);

    const f32 scale_to_fit = (ww / vw) < (wh / vh) ? (ww / vw) : (wh / vh);
    const f32 scaled_w = vw * scale_to_fit;
    const f32 scaled_h = vh * scale_to_fit;
    const f32 off_x = (ww - scaled_w) * 0.5f;
    const f32 off_y = (wh - scaled_h) * 0.5f;

    const f32 sx =  2.0f / vw;
    const f32 sy = -2.0f / vh;

    Mat3 m = Mat3::identity();
    m.m[0] =  sx * scale_to_fit;
    m.m[4] =  sy * scale_to_fit;
    m.m[6] = -1.0f + off_x * (2.0f / ww);
    m.m[7] =  1.0f - off_y * (2.0f / wh);
    return m;
}

} // anonymous namespace

Renderer::Renderer()  = default;
Renderer::~Renderer() { shutdown(); }

void Renderer::init(const InitDesc& desc) {
    sg_desc gfx_desc{};
    gfx_desc.environment = sglue_environment();
    sg_setup(&gfx_desc);

    std::fprintf(stderr, "[renderer] sokol_gfx initialized\n");
    std::fflush(stderr);

    sg_buffer_desc vb_desc{};
    vb_desc.size                  = sizeof(Vtx) * MAX_VERTS;
    vb_desc.usage.vertex_buffer   = true;
    vb_desc.usage.write_transient = true;
    vb_desc.label                 = "batch-vb";
    sg_buffer vb = sg_make_buffer(&vb_desc);
    vertex_buffer_ = reinterpret_cast<void*>(static_cast<uintptr_t>(vb.id));

    sg_shader_desc shd_desc{};
    shd_desc.label                = "batch-shader";
    shd_desc.vertex_func.source   = TRIANGLE_VS;
    shd_desc.fragment_func.source = TRIANGLE_FS;

    sg_pipeline_desc pip_desc{};
    pip_desc.label  = "batch-pipeline";
    pip_desc.shader = sg_make_shader(&shd_desc);

    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[0].offset = 0;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    pip_desc.layout.attrs[1].offset = 8;
    pip_desc.layout.buffers[0].stride = sizeof(Vtx);

    pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pip_desc.cull_mode      = SG_CULLMODE_NONE;

    pip_desc.colors[0].blend.enabled          = true;
    pip_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    pip_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    sg_pipeline pip = sg_make_pipeline(&pip_desc);
    pipeline_ = reinterpret_cast<void*>(static_cast<uintptr_t>(pip.id));

    initialized_ = true;

    std::fprintf(stderr,
        "[renderer] initialized - window %dx%d, virtual %.0fx%.0f, max %d verts\n",
        desc.window_width, desc.window_height, VIRTUAL_W, VIRTUAL_H, MAX_VERTS);
    std::fflush(stderr);
}

void Renderer::shutdown() {
    if (!initialized_) return;
    sg_shutdown();
    initialized_ = false;
    pipeline_      = nullptr;
    vertex_buffer_ = nullptr;
    std::fprintf(stderr, "[renderer] shut down\n");
    std::fflush(stderr);
}

void Renderer::begin_frame(int window_width, int window_height) {
    if (!initialized_) return;

    window_w_   = window_width;
    window_h_   = window_height;
    vert_count_ = 0;

    sg_pass_action pa{};
    pa.colors[0].load_action = SG_LOADACTION_CLEAR;
    pa.colors[0].clear_value = { 0.020f, 0.055f, 0.070f, 1.0f };
    pa.depth.load_action     = SG_LOADACTION_DONTCARE;
    pa.stencil.load_action   = SG_LOADACTION_DONTCARE;

    sg_pass pass{};
    pass.action    = pa;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);
}

void Renderer::end_frame() {
    if (!initialized_) return;

    if (vert_count_ > 0) {
        // Project every vertex from virtual pixels into NDC.
        Mat3 proj = build_projection(window_w_, window_h_);
        for (int i = 0; i < vert_count_; ++i) {
            Vtx& v = cpu_verts_[i];
            Vec2 ndc = proj.transform_point({ v.x, v.y });
            v.x = ndc.x;
            v.y = ndc.y;
        }

        sg_buffer vb{};
        vb.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vertex_buffer_));

        const size_t n_bytes = sizeof(Vtx) * static_cast<size_t>(vert_count_);

        sg_write_buffer_desc wb{};
        wb.dst.buffer    = vb;
        wb.src.data.ptr  = cpu_verts_;
        wb.src.data.size = n_bytes;
        wb.src.offset    = 0;
        wb.size          = n_bytes;
        sg_write_buffer_transient(&wb);

        sg_pipeline pip{};
        pip.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pipeline_));
        sg_apply_pipeline(pip);

        sg_bindings bind{};
        bind.vertex_buffers[0] = vb;
        sg_apply_bindings(&bind);

        sg_draw(0, vert_count_, 1);
    }

    sg_end_pass();
    sg_commit();
}

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------

void Renderer::add_vertex(Vec2 p, Color color) {
    if (vert_count_ >= MAX_VERTS) return;
    f32 r, g, b, a;
    unpack_color(color, r, g, b, a);
    cpu_verts_[vert_count_++] = { p.x, p.y, r, g, b, a };
}

void Renderer::draw_triangle(Vec2 a, Vec2 b, Vec2 c, Color color) {
    add_vertex(a, color);
    add_vertex(b, color);
    add_vertex(c, color);
}

void Renderer::draw_quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color) {
    draw_triangle(a, b, c, color);
    draw_triangle(a, c, d, color);
}

void Renderer::draw_line(Vec2 a, Vec2 b, f32 thickness, Color color) {
    Vec2 diff = b - a;
    const f32 len = diff.length();
    if (len < 1e-6f) return;
    Vec2 dir = diff * (1.0f / len);
    Vec2 perp = dir.perp() * (thickness * 0.5f);

    Vec2 a0 = a + perp;
    Vec2 a1 = a - perp;
    Vec2 b0 = b + perp;
    Vec2 b1 = b - perp;

    draw_quad(a0, b0, b1, a1, color);
}

void Renderer::draw_circle(Vec2 center, f32 radius, Color color, int segments) {
    if (segments < 3) segments = 3;
    const f32 step = TAU / static_cast<f32>(segments);

    for (int i = 0; i < segments; ++i) {
        const f32 a0 = step * static_cast<f32>(i);
        const f32 a1 = a0 + step;
        Vec2 p0 = center + Vec2::from_angle(a0) * radius;
        Vec2 p1 = center + Vec2::from_angle(a1) * radius;
        draw_triangle(center, p0, p1, color);
    }
}

void Renderer::draw_ring(Vec2 center, f32 r_inner, f32 r_outer,
                         Color color, int segments) {
    if (segments < 3) segments = 3;
    const f32 step = TAU / static_cast<f32>(segments);

    for (int i = 0; i < segments; ++i) {
        const f32 a0 = step * static_cast<f32>(i);
        const f32 a1 = a0 + step;

        Vec2 dir0 = Vec2::from_angle(a0);
        Vec2 dir1 = Vec2::from_angle(a1);

        Vec2 in0  = center + dir0 * r_inner;
        Vec2 in1  = center + dir1 * r_inner;
        Vec2 out0 = center + dir0 * r_outer;
        Vec2 out1 = center + dir1 * r_outer;

        draw_quad(in0, in1, out1, out0, color);
    }
}

void Renderer::draw_arc_ring(Vec2 center, f32 r_inner, f32 r_outer,
                             f32 a_start, f32 a_end,
                             Color color, int segments) {
    if (segments < 2) segments = 2;
    const f32 step = (a_end - a_start) / static_cast<f32>(segments);

    for (int i = 0; i < segments; ++i) {
        const f32 a0 = a_start + step * static_cast<f32>(i);
        const f32 a1 = a0 + step;

        Vec2 dir0 = Vec2::from_angle(a0);
        Vec2 dir1 = Vec2::from_angle(a1);

        Vec2 in0  = center + dir0 * r_inner;
        Vec2 in1  = center + dir1 * r_inner;
        Vec2 out0 = center + dir0 * r_outer;
        Vec2 out1 = center + dir1 * r_outer;

        draw_quad(in0, in1, out1, out0, color);
    }
}

} // namespace inc::render