#pragma once

#include "lcycle/World.hpp"
#include "gl/Buffer.hpp"
#include "gl/VArray.hpp"
#include <mathfu/glsl_mappings.h>

namespace gfx {

class WorldRenderer {
using Color = mathfu::vec4;

public:
    WorldRenderer(const lcycle::World& w, const Color& bgColor = Color(1.0, 1.0, 1.0, 1.0));
    void render();

private:
    const lcycle::World& _world;
    gl::Buffer _trails;
    gl::Buffer _cycles;
    gl::Buffer _bg;
    gl::VArray _vao;
    Color _bgColor;
};

}
