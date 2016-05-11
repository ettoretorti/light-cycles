#include "Trail.hpp"
#include "Line.hpp"
#include <vector>
#include <utility>
#include <mathfu/glsl_mappings.h>

namespace lcycle {

Trail::Trail(const mathfu::vec4& color) : _trail(), _color(color) {}
Trail::Trail(const Trail& other) : _trail(other._trail), _color(other._color) {}
Trail::Trail(Trail&& other) : _trail(std::move(other._trail)), _color(std::move(other._color)) {}

Trail& Trail::operator=(const Trail& other) {
    _trail = other._trail;
    _color = other._color;
    return *this;
}

Trail& Trail::operator=(Trail&& other) {
    _trail = std::move(other._trail);
    _color = std::move(other._color);
    return *this;
}

size_t Trail::size() const {
    return _trail.size();
}

Line& Trail::operator[](size_t idx) {
    return _trail[idx];
}

const Line& Trail::operator[](size_t idx) const {
    return _trail[idx];
}

const std::vector<Line>& Trail::data() const {
    return _trail;
}

const mathfu::vec4& Trail::color() const {
    return _color;
}

mathfu::vec4& Trail::color() {
    return _color;
}

void Trail::add(const Line& line) {
    _trail.push_back(line);
}

}
