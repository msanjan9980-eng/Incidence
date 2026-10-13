#include "render/renderer.hpp"
#include "render/shaders.hpp"
#include "render/font.hpp"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#include <cstdio>
#include <cstring>

namespace inc::render {

namespace {

inline void unpack_color(Color c, f32& out_r, f32& out_g, f32& out_b, f32& out_a) {
    constexpr f32 inv255 = 1.0f / 255.0f;
    out_r = f32(color_r(c)) * inv255;
    out_g = f32(color_g(c)) * inv255;
    out_b = f32(color_b(c)) * inv255;
    out_a = f32(color_a(c)) * inv255;
}

Mat3 build_projection(int window_w, int window_h) {
    const f32 vw = VIRTUAL_W;
    const f32 vh = VIRTUAL_H;
    const f32 ww = f32(window_w);
    const f32 wh = f32(window_h);

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

sg_pipeline make_batch_pipeline(sg_shader shader, bool additive) {
    sg_pipeline_desc desc{};
    desc.label  = additive ? "batch-pipeline-add" : "batch-pipeline-alpha";
    desc.shader = shader;

    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[1].offset = 8;
    desc.layout.buffers[0].stride = sizeof(f32) * 6;

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc.cull_mode      = SG_CULLMODE_NONE;

    desc.colors[0].blend.enabled          = true;
    desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb   = additive
        ? SG_BLENDFACTOR_ONE
        : SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    desc.colors[0].blend.dst_factor_alpha = additive
        ? SG_BLENDFACTOR_ONE
        : SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    return sg_make_pipeline(&desc);
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
    sg_shader shader = sg_make_shader(&shd_desc);

    sg_pipeline pip_a = make_batch_pipeline(shader, false);
    pipeline_alpha_ = reinterpret_cast<void*>(static_cast<uintptr_t>(pip_a.id));

    sg_pipeline pip_add = make_batch_pipeline(shader, true);
    pipeline_additive_ = reinterpret_cast<void*>(static_cast<uintptr_t>(pip_add.id));

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
    pipeline_alpha_    = nullptr;
    pipeline_additive_ = nullptr;
    vertex_buffer_     = nullptr;
    std::fprintf(stderr, "[renderer] shut down\n");
    std::fflush(stderr);
}

void Renderer::begin_frame(int window_width, int window_height) {
    if (!initialized_) return;

    window_w_           = window_width;
    window_h_           = window_height;
    vert_count_         = 0;
    batch_count_        = 0;
    current_batch_start_= 0;
    current_layer_      = Layer::Alpha;

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

void Renderer::set_layer(Layer l) {
    if (l == current_layer_) return;
    if (vert_count_ > current_batch_start_ && batch_count_ < MAX_BATCHES) {
        batches_[batch_count_++] = {
            current_batch_start_,
            vert_count_ - current_batch_start_,
            current_layer_
        };
    }
    current_batch_start_ = vert_count_;
    current_layer_       = l;
}

void Renderer::end_frame() {
    if (!initialized_) return;

    if (vert_count_ > current_batch_start_ && batch_count_ < MAX_BATCHES) {
        batches_[batch_count_++] = {
            current_batch_start_,
            vert_count_ - current_batch_start_,
            current_layer_
        };
    }

    if (vert_count_ > 0 && batch_count_ > 0) {
        Mat3 proj = build_projection(window_w_, window_h_);
        for (int i = 0; i < vert_count_; ++i) {
            Vtx& v = cpu_verts_[i];
            Vec2 ndc = proj.transform_point({ v.x, v.y });
            v.x = ndc.x;
            v.y = ndc.y;
        }

        sg_buffer vb{};
        vb.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vertex_buffer_));

        const size_t n_bytes = sizeof(Vtx) * size_t(vert_count_);
        sg_write_buffer_desc wb{};
        wb.dst.buffer    = vb;
        wb.src.data.ptr  = cpu_verts_;
        wb.src.data.size = n_bytes;
        wb.src.offset    = 0;
        wb.size          = n_bytes;
        sg_write_buffer_transient(&wb);

        sg_bindings bind{};
        bind.vertex_buffers[0] = vb;

        for (int i = 0; i < batch_count_; ++i) {
            const Batch& b = batches_[i];

            sg_pipeline pip{};
            pip.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
                b.layer == Layer::Alpha ? pipeline_alpha_ : pipeline_additive_));
            sg_apply_pipeline(pip);

            sg_apply_bindings(&bind);

            sg_draw(b.start, b.count, 1);
        }
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
    add_vertex(a, color); add_vertex(b, color); add_vertex(c, color);
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
    draw_quad(a + perp, b + perp, b - perp, a - perp, color);
}

void Renderer::draw_circle(Vec2 center, f32 radius, Color color, int segments) {
    if (segments < 3) segments = 3;
    const f32 step = TAU / f32(segments);
    for (int i = 0; i < segments; ++i) {
        const f32 a0 = step * f32(i);
        const f32 a1 = a0 + step;
        draw_triangle(center,
                      center + Vec2::from_angle(a0) * radius,
                      center + Vec2::from_angle(a1) * radius,
                      color);
    }
}

void Renderer::draw_ring(Vec2 center, f32 r_inner, f32 r_outer,
                         Color color, int segments) {
    if (segments < 3) segments = 3;
    const f32 step = TAU / f32(segments);
    for (int i = 0; i < segments; ++i) {
        const f32 a0 = step * f32(i);
        const f32 a1 = a0 + step;
        Vec2 d0 = Vec2::from_angle(a0);
        Vec2 d1 = Vec2::from_angle(a1);
        draw_quad(center + d0 * r_inner,
                  center + d1 * r_inner,
                  center + d1 * r_outer,
                  center + d0 * r_outer,
                  color);
    }
}

void Renderer::draw_arc_ring(Vec2 center, f32 r_inner, f32 r_outer,
                             f32 a_start, f32 a_end,
                             Color color, int segments) {
    if (segments < 2) segments = 2;
    const f32 step = (a_end - a_start) / f32(segments);
    for (int i = 0; i < segments; ++i) {
        const f32 a0 = a_start + step * f32(i);
        const f32 a1 = a0 + step;
        Vec2 d0 = Vec2::from_angle(a0);
        Vec2 d1 = Vec2::from_angle(a1);
        draw_quad(center + d0 * r_inner,
                  center + d1 * r_inner,
                  center + d1 * r_outer,
                  center + d0 * r_outer,
                  color);
    }
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

f32 Renderer::text_width(const char* str, f32 pixel_size) const {
    if (!str) return 0.0f;
    int n = 0;
    while (str[n]) ++n;
    if (n == 0) return 0.0f;
    // n glyphs, each `FONT_ADVANCE` wide, minus one trailing advance.
    return (f32(n) * f32(FONT_ADVANCE) - 1.0f) * pixel_size;
}

void Renderer::draw_text(Vec2 pos, const char* str, f32 pixel_size, Color color) {
    if (!str) return;

    const f32 px = pixel_size;
    f32 x = pos.x;

    while (*str) {
        const unsigned char c = static_cast<unsigned char>(*str);
        if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) {
            x += f32(FONT_ADVANCE) * px;
            ++str;
            continue;
        }

        const u8* glyph = FONT_5X7[c - FONT_FIRST_CHAR];
        for (int row = 0; row < FONT_GLYPH_HEIGHT; ++row) {
            const u8 bits = glyph[row];
            for (int col = 0; col < FONT_GLYPH_WIDTH; ++col) {
                if (bits & (1 << (FONT_GLYPH_WIDTH - 1 - col))) {
                    const f32 gx = x + f32(col) * px;
                    const f32 gy = pos.y + f32(row) * px;
                    draw_quad({ gx,           gy           },
                              { gx + px,      gy           },
                              { gx + px,      gy + px      },
                              { gx,           gy + px      },
                              color);
                }
            }
        }

        x += f32(FONT_ADVANCE) * px;
        ++str;
    }
}

void Renderer::draw_text_centered(Vec2 pos, const char* str,
                                  f32 pixel_size, Color color) {
    const f32 w = text_width(str, pixel_size);
    draw_text({ pos.x - w * 0.5f, pos.y }, str, pixel_size, color);
}

} // namespace inc::render