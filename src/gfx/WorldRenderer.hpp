#pragma once

#include <mathfu/glsl_mappings.h>
#include <vector>
#include "gl/Buffer.hpp"
#include "gl/VArray.hpp"
#include "lcycle/World.hpp"

namespace gfx {

constexpr GLfloat BG_COLOR[4] = {1.0, 1.0, 1.0, 1.0};

class WorldRenderer {
   public:
    WorldRenderer();
    void render(const lcycle::World& w);

   private:
    gl::Buffer _trails;
    gl::Buffer _cycles;
    gl::Buffer _bg;
    gl::VArray _vao;
    std::vector<GLfloat> _buf;
};

}  // namespace gfx
