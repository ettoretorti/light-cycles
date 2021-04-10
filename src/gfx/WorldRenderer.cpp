#include "WorldRenderer.hpp"

#include <mathfu/glsl_mappings.h>

#include <algorithm>

namespace gfx {

WorldRenderer::WorldRenderer()
    : _trails(GL_ARRAY_BUFFER), _cycles(GL_ARRAY_BUFFER), _bg(GL_ARRAY_BUFFER), _vao(), _buf() {}

void WorldRenderer::render(const lcycle::World& w) {
    const float sizeDiv2 = w.size() / 2.0;
    const GLfloat bgVecs[] = {-sizeDiv2, -sizeDiv2, -sizeDiv2, sizeDiv2, sizeDiv2, sizeDiv2, sizeDiv2, -sizeDiv2};
    _bg.data(sizeof(bgVecs), &bgVecs, GL_STATIC_DRAW);

    // Update the buffers
    const auto& players = w.players();
    const auto& trails = w.trails();

    size_t nLines = 0;
    for (const auto& trail : trails) nLines += trail.size();

    const size_t bufSize = 4 * std::max(players.size(), nLines);
    _buf.resize(bufSize);
    GLfloat* buf = _buf.data();

    for (size_t i = 0; i < players.size(); i++) {
        const auto line = players[i].cycle.toLine();

        buf[4 * i + 0] = line.start().x();
        buf[4 * i + 1] = line.start().y();

        buf[4 * i + 2] = line.end().x();
        buf[4 * i + 3] = line.end().y();
    }

    _cycles.data(4 * players.size() * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);
    _cycles.subData(0, 4 * players.size() * sizeof(GLfloat), buf);

    size_t curOffset = 0;
    for (auto& trail : trails) {
        for (size_t i = 0; i < trail.size(); i++) {
            buf[curOffset + 0] = trail[i].start().x();
            buf[curOffset + 1] = trail[i].start().y();

            buf[curOffset + 2] = trail[i].end().x();
            buf[curOffset + 3] = trail[i].end().y();

            curOffset += 4;
        }
    }

    _trails.data(4 * nLines * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);
    _trails.subData(0, 4 * nLines * sizeof(GLfloat), buf);

    // Render
    _vao.bind();
    _vao.enableVertexAttrib(0);  // vertices

    _bg.bind();
    _vao.vertexAttribPointer(0, 2, GL_FLOAT);
    glVertexAttrib4fv(1, BG_COLOR);  // color
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    _cycles.bind();
    _vao.vertexAttribPointer(0, 2, GL_FLOAT);
    size_t curStart = 0;
    for (const auto& player : players) {
        glVertexAttrib4fv(1, &player.color[0]);
        glDrawArrays(GL_LINES, curStart, 2);
        curStart += 2;
    }

    _trails.bind();
    _vao.vertexAttribPointer(0, 2, GL_FLOAT);
    curStart = 0;
    for (const auto& trail : trails) {
        glVertexAttrib4fv(1, &(trail.color()[0]));
        glDrawArrays(GL_LINES, curStart, 2 * trail.size());
        curStart += 2 * trail.size();
    }
}

}  // namespace gfx
