#pragma once

// ---------------------------------------------------------------------------
// DIAGNOSTIC shaders. No uniforms. Vertices are in NDC (-1..1).
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