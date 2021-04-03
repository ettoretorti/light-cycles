#pragma once

#include "gui/nuklear.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl/Buffer.hpp"
#include "gl/VArray.hpp"
#include "gl/Program.hpp"

namespace gui {

class GuiState {
public:
    GuiState();
    ~GuiState();

    void startInput();
    void endInput();

    void render(GLFWwindow* win);

    nk_context ctx;
    bool active;
private:
    nk_font_atlas _atlas;
    nk_draw_null_texture _null;
    nk_buffer _cmdbuf, _vbuf, _ebuf;
    gl::Program _p;
    gl::VArray _vao;
    gl::Buffer _vbo;
    gl::Buffer _ebo;
    GLuint _fontTexture;
};

}
