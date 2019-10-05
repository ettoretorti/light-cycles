#pragma once

#include "lcycle/World.hpp"
#include "gl/Buffer.hpp"
#include "gl/VArray.hpp"
#include <mathfu/glsl_mappings.h>
#include <vector>

namespace gfx {

constexpr GLfloat BG_COLOR[4] = { 1.0, 1.0, 1.0, 1.0 };

class WorldRenderer {
public:
    WorldRenderer(const lcycle::World& w);
    void render();

private:
    const lcycle::World& _world;
    gl::Buffer _trails;
    gl::Buffer _cycles;
    gl::Buffer _bg;
    gl::VArray _vao;
    std::vector<GLfloat> _buf;
};

}
