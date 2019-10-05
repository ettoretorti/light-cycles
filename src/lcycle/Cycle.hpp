#pragma once

#include "Line.hpp"

#include <mathfu/glsl_mappings.h>
#include <functional>

namespace lcycle {

const double CYCLE_SPEED = 6.0;
const double CYCLE_LENGTH = 0.9;
const double TURN_SPEED  =  90 * 3.14159;

struct CycleInput {
    /*! -1 for left, 1 for right. Automatically clampedd to that range. */
    float turnDir;
};

class Cycle {
public:
    Cycle(const mathfu::vec2& pos, double orientation);
    void runFor(double secs);
    void rotate(double degreesCounterClockwise);
    Line toLine() const;

private:
    mathfu::vec2 _pos;
    double _orientation;
};

}
