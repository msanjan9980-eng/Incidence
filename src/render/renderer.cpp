#include "render/renderer.hpp"
#include "render/shaders.hpp"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#include <cstdio>

namespace inc::render {

namespace {

// DIAGNOSTIC — bare-minimum vertex: position only, in NDC (-1..1).
// No color, no transform, no uniforms. If this doesn't render, the bug
// is not in our game math — it is in the pipeline setup.
struct Vertex {
    f32 x, y;      // NDC position
    f32 r, g, b, a;
};

// A large triangle occupying most of the screen in NDC. Fully opaque cyan.
// This triangle is hardcoded to prove the pipeline works at the lowest level.
constexpr Vertex TRIANGLE_VERTS[3] = {
    {  0.00f,  0.80f,   1.00f, 1.00f, 0.00f, 1.0f },  // top     — bright cyan
    { -0.80f, -0.80f,   1.00f, 1.00f, 0.00f, 1.0f },  // bottom-L — bright cyan
    {  0.80f, -0.80f,   1.00f, 1.00f, 0.00f, 1.0f },  // bottom-R — bright cyan
};

void mat3_to_mat4(const Mat3& m, f32 out[16]) {
    out[0]  = m.m[0]; out[1]  = m.m[1]; out[2]  = 0.0f; out[3]  = 0.0f;
    out[4]  = m.m[3]; out[5]  = m.m[4]; out[6]  = 0.0f; out[7]  = 0.0f;
    out[8]  = 0.0f;   out[9]  = 0.0f;   out[10] = 1.0f; out[11] = 0.0f;
    out[12] = m.m[6]; out[13] = m.m[7]; out[14] = 0.0f; out[15] = 1.0f;
}

} // anonymous namespace

Renderer::Renderer()  = default;
Renderer::~Renderer() { shutdown(); }

void Renderer::init(const InitDesc& desc) {
    sg_desc gfx_desc{};
    gfx_desc.environment = sglue_environment();
    sg_setup(&gfx_desc);

    std::fprintf(stderr, "[renderer] sokol_gfx initialized\n"); std::fflush(stderr);

    sg_buffer_desc vb_desc{};
    vb_desc.data = SG_RANGE(TRIANGLE_VERTS);
    vb_desc.label = "triangle-vb";
    sg_buffer vb = sg_make_buffer(&vb_desc);
    vertex_buffer_ = reinterpret_cast<void*>(static_cast<uintptr_t>(vb.id));

    sg_pipeline_desc pip_desc{};
    pip_desc.label = "triangle-pipeline";
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[0].offset = 0;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    pip_desc.layout.attrs[1].offset = 8;
    pip_desc.layout.buffers[0].stride = sizeof(Vertex);

    sg_shader_desc shd_desc{};
    shd_desc.label = "triangle-shader";
    shd_desc.vertex_func.source   = TRIANGLE_VS;
    shd_desc.fragment_func.source = TRIANGLE_FS;
    // No uniform blocks — this pipeline has no uniforms at all.
    pip_desc.shader = sg_make_shader(&shd_desc);

    pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    // Explicitly disable culling so winding order cannot be the cause of an
    // invisible triangle.
    pip_desc.cull_mode = SG_CULLMODE_NONE;
    pip_desc.colors[0].blend.enabled = true;
    pip_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    pip_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    sg_pipeline pip = sg_make_pipeline(&pip_desc);
    pipeline_ = reinterpret_cast<void*>(static_cast<uintptr_t>(pip.id));

    initialized_ = true;

    std::fprintf(stderr,
        "[renderer] initialized - window %dx%d, virtual %.0fx%.0f\n",
        desc.window_width, desc.window_height, VIRTUAL_W, VIRTUAL_H);
    std::fflush(stderr);
}

void Renderer::shutdown() {
    if (!initialized_) return;
    sg_shutdown();
    initialized_ = false;
    pipeline_     = nullptr;
    vertex_buffer_= nullptr;
    std::fprintf(stderr, "[renderer] shut down\n"); std::fflush(stderr);
}

void Renderer::begin_frame(int window_width, int window_height) {
    if (!initialized_) return;

    sg_pass_action pa{};
    pa.colors[0].load_action  = SG_LOADACTION_CLEAR;
    pa.colors[0].clear_value  = { 0.05f, 0.05f, 0.10f, 1.0f };  // dark blue
    pa.depth.load_action      = SG_LOADACTION_DONTCARE;
    pa.stencil.load_action    = SG_LOADACTION_DONTCARE;

    sg_pass pass{};
    pass.action    = pa;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    (void)window_width; (void)window_height;
}

void Renderer::draw_triangle(const Mat3& /*transform*/,
                             f32 /*r*/, f32 /*g*/, f32 /*b*/, f32 /*a*/) {
    if (!initialized_) return;

    sg_pipeline pip{};
    pip.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pipeline_));
    sg_apply_pipeline(pip);

    sg_buffer vb{};
    vb.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vertex_buffer_));

    sg_bindings bind{};
    bind.vertex_buffers[0] = vb;
    sg_apply_bindings(&bind);

    // No uniforms to upload.
    sg_draw(0, 3, 1);
}

void Renderer::end_frame() {
    if (!initialized_) return;
    sg_end_pass();
    sg_commit();
}

} // namespace inc::render