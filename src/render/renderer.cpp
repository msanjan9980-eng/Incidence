#include "render/renderer.hpp"
#include "render/shaders.hpp"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#include <cstdio>

namespace inc::render {

namespace {

// Vertex format: NDC position + RGBA color.
struct Vertex {
    f32 x, y;
    f32 r, g, b, a;
};

// Triangle in LOCAL pixel coordinates, centered on the origin.
// The model matrix places and rotates it; the projection maps to NDC.
constexpr Vertex TRIANGLE_VERTS[3] = {
    {   0.0f, -40.0f,   0.10f, 0.85f, 1.00f, 1.0f },  // top      -- bright cyan
    { -35.0f,  30.0f,   0.10f, 0.85f, 1.00f, 1.0f },  // bottom-L -- bright cyan
    {  35.0f,  30.0f,   0.05f, 0.55f, 0.85f, 1.0f },  // bottom-R -- deep cyan
};

// Build the projection matrix that maps virtual pixels (0,0 at top-left)
// into NDC (-1..1) with a letterbox fit inside the actual window.
Mat3 make_projection(int window_w, int window_h) {
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
    // ---- 1. Initialize sokol_gfx -------------------------------------------
    sg_desc gfx_desc{};
    gfx_desc.environment = sglue_environment();
    sg_setup(&gfx_desc);

    std::fprintf(stderr, "[renderer] sokol_gfx initialized\n");
    std::fflush(stderr);

    // ---- 2. Create a dynamic vertex buffer --------------------------------
    // - vertex_buffer: this buffer holds vertex data
    // - write_transient: contents are rewritten every frame
    // New sokol_gfx API: see sg_buffer_usage in sokol_gfx.h
    sg_buffer_desc vb_desc{};
    vb_desc.size                = sizeof(Vertex) * 3;
    vb_desc.usage.vertex_buffer = true;
    vb_desc.usage.write_transient = true;
    vb_desc.label               = "triangle-dyn-vb";
    sg_buffer vb = sg_make_buffer(&vb_desc);
    vertex_buffer_ = reinterpret_cast<void*>(static_cast<uintptr_t>(vb.id));

    // ---- 3. Create the shader (no uniforms) -------------------------------
    sg_shader_desc shd_desc{};
    shd_desc.label = "triangle-shader";
    shd_desc.vertex_func.source   = TRIANGLE_VS;
    shd_desc.fragment_func.source = TRIANGLE_FS;

    // ---- 4. Build the pipeline -------------------------------------------
    sg_pipeline_desc pip_desc{};
    pip_desc.label  = "triangle-pipeline";
    pip_desc.shader = sg_make_shader(&shd_desc);

    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[0].offset = 0;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    pip_desc.layout.attrs[1].offset = 8;
    pip_desc.layout.buffers[0].stride = sizeof(Vertex);

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
        "[renderer] initialized - window %dx%d, virtual %.0fx%.0f\n",
        desc.window_width, desc.window_height, VIRTUAL_W, VIRTUAL_H);
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

void Renderer::begin_frame(int /*window_width*/, int /*window_height*/) {
    if (!initialized_) return;

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

void Renderer::draw_triangle(const Mat3& transform,
                             f32 /*r*/, f32 /*g*/, f32 /*b*/, f32 /*a*/) {
    if (!initialized_) return;

    // ---- 1. Compute MVP = projection * model ------------------------------
    Mat3 proj = make_projection(sapp_width(), sapp_height());
    Mat3 mvp  = proj * transform;

    // ---- 2. Transform the 3 vertices on the CPU ---------------------------
    Vertex ndc_verts[3];
    for (int i = 0; i < 3; ++i) {
        Vec2 local { TRIANGLE_VERTS[i].x, TRIANGLE_VERTS[i].y };
        Vec2 ndc   = mvp.transform_point(local);
        ndc_verts[i] = {
            ndc.x, ndc.y,
            TRIANGLE_VERTS[i].r, TRIANGLE_VERTS[i].g,
            TRIANGLE_VERTS[i].b, TRIANGLE_VERTS[i].a
        };
    }

    // ---- 3. Upload transformed vertices (new sokol_gfx API) ---------------
    // sg_write_buffer_transient: writes the whole buffer once per frame.
    // No seal needed; sokol handles slot rotation internally.
    sg_buffer vb{};
    vb.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vertex_buffer_));

    sg_write_buffer_desc wb{};
    wb.dst.buffer    = vb;
    wb.src.data.ptr  = ndc_verts;
    wb.src.data.size = sizeof(ndc_verts);
    wb.src.offset    = 0;
    wb.size          = sizeof(ndc_verts);
    sg_write_buffer_transient(&wb);

    // ---- 4. Draw ----------------------------------------------------------
    sg_pipeline pip{};
    pip.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pipeline_));
    sg_apply_pipeline(pip);

    sg_bindings bind{};
    bind.vertex_buffers[0] = vb;
    sg_apply_bindings(&bind);

    sg_draw(0, 3, 1);
}

void Renderer::end_frame() {
    if (!initialized_) return;
    sg_end_pass();
    sg_commit();
}

} // namespace inc::render