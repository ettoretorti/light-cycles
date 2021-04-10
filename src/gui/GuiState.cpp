#include "gui/GuiState.hpp"

#include <stdexcept>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <mathfu/glsl_mappings.h>

#include "gl/Buffer.hpp"
#include "gl/Program.hpp"
#include "gl/Shader.hpp"
#include "gl/VArray.hpp"
#include "gui/nuklear.h"

namespace {

constexpr GLuint kPosAttribIdx = 0;
constexpr GLuint kUvAttribIdx = 1;
constexpr GLuint kColorAttribIdx = 2;

struct VertexData {
    GLfloat pos[2];
    GLfloat uv[2];
    GLbyte col[4];
};

constexpr char kVertexShader[] = R"glsl(
    #version 330
    uniform mat4 mvp;
    in vec2 pos;
    in vec2 uv;
    in vec4 color;
    out vec2 fUv;
    out vec4 fColor;

    void main() {
        fUv = uv;
        fColor = color;
        gl_Position = mvp * vec4(pos.xy, 0, 1);
    }
)glsl";

constexpr char kFragmentShader[] = R"glsl(
    #version 330
    uniform  sampler2D tex;
    in vec2 fUv;
    in vec4 fColor;
    out vec4 color;
    void main() {
        color = fColor * texture(tex, fUv.st);
    }
)glsl";

GLuint createFontTexture(const void* data, int w, int h) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return texture;
}

}  // namespace

namespace gui {

GuiState::GuiState()
    : active(false), _p(), _vao(), _vbo(GL_ARRAY_BUFFER), _ebo(GL_ELEMENT_ARRAY_BUFFER), _fontTexture(0) {
    nk_buffer_init_default(&_cmdbuf);
    nk_buffer_init_default(&_vbuf);
    nk_buffer_init_default(&_ebuf);

    nk_font_atlas_init_default(&_atlas);
    nk_font_atlas_begin(&_atlas);
    nk_font* font = nk_font_atlas_add_default(&_atlas, 13, nullptr);
    int w, h;
    const void* fontImage = nk_font_atlas_bake(&_atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    _fontTexture = createFontTexture(fontImage, w, h);
    nk_font_atlas_end(&_atlas, nk_handle_id((int)_fontTexture), &_null);
    nk_font_atlas_cleanup(&_atlas);

    if (!nk_init_default(&ctx, &font->handle)) {
        throw std::runtime_error("Failed to initialize nuklear context");
    }

    gl::Shader vs(GL_VERTEX_SHADER);
    vs.source(kVertexShader);
    if (!vs.compile()) {
        throw std::runtime_error("Could not compile vertex shader:\n" + vs.infoLog());
    }
    gl::Shader fs(GL_FRAGMENT_SHADER);
    fs.source(kFragmentShader);
    if (!fs.compile()) {
        throw std::runtime_error("Could not compile fragment shader:\n" + fs.infoLog());
    }

    _p.setVertexShader(vs);
    _p.setFragmentShader(fs);
    _p.bindAttrib(kPosAttribIdx, "pos");
    _p.bindAttrib(kUvAttribIdx, "uv");
    _p.bindAttrib(kColorAttribIdx, "color");
    if (!_p.link()) {
        throw std::runtime_error("Could not link program:\n" + _p.infoLog());
    }

    _vao.bind();
    _vao.enableVertexAttrib(kPosAttribIdx);
    _vao.enableVertexAttrib(kUvAttribIdx);
    _vao.enableVertexAttrib(kColorAttribIdx);

    _vbo.bind();
    _vao.vertexAttribPointer(kPosAttribIdx, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData),
                             (void*)offsetof(VertexData, pos));
    _vao.vertexAttribPointer(kUvAttribIdx, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, uv));
    _vao.vertexAttribPointer(kColorAttribIdx, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData),
                             (void*)offsetof(VertexData, col));
}

GuiState::~GuiState() {
    nk_font_atlas_clear(&_atlas);
    nk_buffer_free(&_cmdbuf);
    nk_buffer_free(&_vbuf);
    nk_buffer_free(&_ebuf);
    nk_free(&ctx);
    glDeleteTextures(1, &_fontTexture);
}

void GuiState::startInput() { nk_input_begin(&ctx); }

void GuiState::endInput() { nk_input_end(&ctx); }

void GuiState::render(GLFWwindow* win) {
    constexpr nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(VertexData, pos)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(VertexData, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(VertexData, col)},
        {NK_VERTEX_LAYOUT_END},
    };

    nk_convert_config cfg;
    cfg.global_alpha = 0.75;
    cfg.line_AA = cfg.shape_AA = NK_ANTI_ALIASING_ON;
    cfg.circle_segment_count = cfg.arc_segment_count = cfg.curve_segment_count = 22;
    cfg.null = _null;
    cfg.vertex_layout = vertex_layout;
    cfg.vertex_size = sizeof(VertexData);
    cfg.vertex_alignment = alignof(VertexData);

    if (nk_convert(&ctx, &_cmdbuf, &_vbuf, &_ebuf, &cfg) != NK_CONVERT_SUCCESS) {
        throw new std::runtime_error("Failed to convert gui display lists to vertex data");
    }

    _vbo.data(_vbuf.allocated, nullptr, GL_STREAM_DRAW);
    _vbo.subData(0, _vbuf.allocated, nk_buffer_memory(&_vbuf));

    _ebo.data(_ebuf.allocated, nullptr, GL_STREAM_DRAW);
    _ebo.subData(0, _ebuf.allocated, nk_buffer_memory(&_ebuf));

    int w, h;
    glfwGetFramebufferSize(win, &w, &h);
    const auto proj = mathfu::mat4::Ortho(0, w, 0, h, -1.0, 1.0);
    auto flip = mathfu::mat4::Identity();
    flip(1, 1) = -1.0;
    const auto mvp = flip * proj;
    _p.use();
    _vao.bind();
    _vbo.bind();
    _ebo.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    glUniform1i(_p.getUniform("tex"), 0);
    glUniformMatrix4fv(_p.getUniform("mvp"), 1, false, &mvp[0]);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    const nk_draw_command* cmd;
    const nk_draw_index* offset = nullptr;
    nk_draw_foreach(cmd, &ctx, &_cmdbuf) {
        if (cmd->elem_count == 0) {
            continue;
        }
        glBindTexture(GL_TEXTURE_2D, cmd->texture.id);
        glScissor(cmd->clip_rect.x, h - (cmd->clip_rect.y + cmd->clip_rect.h), cmd->clip_rect.w, cmd->clip_rect.h);
        glDrawElements(GL_TRIANGLES, cmd->elem_count, GL_UNSIGNED_SHORT, (void*)offset);
        offset += cmd->elem_count;
    }

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    nk_clear(&ctx);
    nk_buffer_clear(&_cmdbuf);
    nk_buffer_clear(&_vbuf);
    nk_buffer_clear(&_ebuf);
}

}  // namespace gui
