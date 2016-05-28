#include "Cycle.hpp"

#include <mathfu/glsl_mappings.h>
#include <functional>

#include <cmath>


namespace lcycle {

Cycle::Cycle(const mathfu::vec2& pos, double orientation)
    : _pos(pos), _orientation(orientation)
{}

void Cycle::runFor(double secs) {
    _pos =  _pos + (float)(secs * CYCLE_SPEED) * mathfu::vec2(cos(_orientation), sin(_orientation));
}

void Cycle::rotate(double dCC) {
    double newO = _orientation + dCC * M_PI / 180.0;
    _orientation = fmod(newO, 2 * M_PI);
}

Line Cycle::toLine() const {
	using namespace mathfu;

	vec2 dir = vec2(cos(_orientation), sin(_orientation));

	return Line(_pos - (float)(CYCLE_LENGTH/2.0) * dir, _pos + (float)(CYCLE_LENGTH/2.0) * dir);
}

}
