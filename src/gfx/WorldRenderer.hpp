#pragma once

#include "lcycle/World.hpp"
#include "gl/Buffer.hpp"
#include "gl/VArray.hpp"
#include <mathfu/glsl_mappings.h>

namespace gfx {

class WorldRenderer {
using Color = mathfu::vec4;

public: 
    static const Color TRAIL_COLOR;
    static const Color CYCLE_COLOR;
    static const Color BG_COLOR;

    WorldRenderer(const lcycle::World& w);
    void render();

private:
    const lcycle::World& _world;
    gl::Buffer _trails;
    gl::Buffer _cycles;
    gl::Buffer _bg;
    gl::VArray _vao;
};

}
