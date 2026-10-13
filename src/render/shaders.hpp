#pragma once

// ---------------------------------------------------------------------------
// Shader sources for INCIDENCE.
//
// These shaders are intentionally trivial: the vertex shader passes through
// pre-transformed NDC positions and colours. All world-to-NDC math happens
// on the CPU (in renderer.cpp), and the resulting NDC vertices are uploaded
// to a dynamic vertex buffer every frame.
//
// Why CPU transform?
//   - It works identically on every sokol_gfx backend (GL, D3D11, Metal, WebGPU).
//   - It avoids std140 uniform-block layout pitfalls.
//   - For a 2D game with ~50-200 entities, the CPU cost is negligible.
//
// If we later need GPU-side transforms for particles or tens of thousands of
// sprites, we can introduce a second pipeline with a proper uniform block.
// For now, this is the correct trade-off.
//
// Vertex format:
//   attribute 0 : vec2 a_pos     -- already in NDC (-1..1)
//   attribute 1 : vec4 a_color   -- RGBA 0..1
// ---------------------------------------------------------------------------

inline constexpr const char* TRIANGLE_VS = R"(
#version 410 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec4 a_color;
out vec4 v_color;
void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
    v_color = a_color;
}
)";

inline constexpr const char* TRIANGLE_FS = R"(
#version 410 core
in vec4 v_color;
out vec4 frag_color;
void main() {
    frag_color = v_color;
}
)";