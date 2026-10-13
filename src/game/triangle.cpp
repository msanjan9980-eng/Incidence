#include "game/triangle.hpp"
#include "render/renderer.hpp"

#include <cmath>

namespace inc::game {

void Triangle::step(f32 dt,
                    bool key_left, bool key_right,
                    bool key_up,   bool key_down,
                    bool key_q,    bool key_e)
{
    // ---- Arrow keys: accelerate in the requested direction ---------------
    f32 ax = 0.0f, ay = 0.0f;
    if (key_left)  ax -= 1.0f;
    if (key_right) ax += 1.0f;
    if (key_up)    ay -= 1.0f;
    if (key_down)  ay += 1.0f;

    // Normalise diagonal input so holding two arrows doesn't go 1.41x faster.
    const f32 len = std::sqrt(ax * ax + ay * ay);
    if (len > 0.0f) { ax /= len; ay /= len; }

    velocity.x += ax * ACCEL * dt;
    velocity.y += ay * ACCEL * dt;

    // ---- Drag: exponential decay, framerate-independent -------------------
    const f32 drag_factor = std::pow(DRAG, dt);
    velocity.x *= drag_factor;
    velocity.y *= drag_factor;

    // ---- Speed cap -------------------------------------------------------
    const f32 speed = velocity.length();
    if (speed > MAX_SPEED) {
        velocity = velocity * (MAX_SPEED / speed);
    }

    // ---- Integrate position ----------------------------------------------
    position += velocity * dt;

    // ---- Bounce off virtual-canvas edges --------------------------------
    // We bounce the *centre* of the triangle at RADIUS from each edge.
    // On hit, we reverse the velocity component AND push the position back
    // inside so the next frame doesn't re-trigger the same hit.
    if (position.x < RADIUS) {
        position.x = RADIUS;
        velocity.x = -velocity.x;
    } else if (position.x > VIRTUAL_W - RADIUS) {
        position.x = VIRTUAL_W - RADIUS;
        velocity.x = -velocity.x;
    }
    if (position.y < RADIUS) {
        position.y = RADIUS;
        velocity.y = -velocity.y;
    } else if (position.y > VIRTUAL_H - RADIUS) {
        position.y = VIRTUAL_H - RADIUS;
        velocity.y = -velocity.y;
    }

    // ---- Rotation from Q / E ---------------------------------------------
    if (key_q) spin = -SPIN_SPEED;
    else if (key_e) spin = SPIN_SPEED;
    else spin *= std::pow(0.02f, dt);  // rapid spin-down when keys released

    angle += spin * dt;
}

void Triangle::draw(render::Renderer& r) const {
    // Build a model matrix: translate(position) * rotate(angle)
    Mat3 model = Mat3::translation(position) * Mat3::rotation(angle);
    r.draw_triangle(model, 1.0f, 1.0f, 1.0f, 1.0f);
}

} // namespace inc::game