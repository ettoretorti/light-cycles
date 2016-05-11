#include "Line.hpp"
#include <mathfu/glsl_mappings.h>


namespace lcycle {

Line::Line(const mathfu::vec2& start, const mathfu::vec2& end) : _s(start), _e(end) {}
Line::Line(const Line& other) : _s(other._s), _e(other._e) {}
Line::Line(Line&& other) : _s(other._s), _e(other._e) {}

Line& Line::operator=(const Line& other) {
    _s = other._s;
    _e = other._e;
    return *this;
}

Line& Line::operator=(Line&& other) {
    _s = other._s;
    _e = other._e;
    return *this;
}

const mathfu::vec2& Line::start() const {
    return _s;
}

const mathfu::vec2& Line::end() const {
    return _e;
}

mathfu::vec2& Line::start() {
    return _s;
}

mathfu::vec2& Line::end() {
    return _e;
}

double Line::len() const {
    return (_s - _e).Length();
}

double Line::lenSquared() const {
    return (_s - _e).LengthSquared();
}

// doesn't currently detect coincident lines as intersecting
bool Line::intersect(const Line& l1, const Line& l2) {
    using namespace mathfu;

    vec2 x = l1.start();
    vec2 a = l1.end() - x;
    
    vec2 y = l2.start();
    vec2 b = l2.end() - y;
    
    //  a0(y1 - x1) - a1(y0 - x0)
    //  -------------------------
    //      a1 * b0 - a0 * b1
    
    double denominator = (a.y() * b.x() - a.x() * b.y());
    
    //parallel or coincident
    if(denominator == 0.0) return false;
    
    double mu = (a.x() * (y.y() - x.y()) - a.y() * (y.x() - x.x())) / denominator;
    double lambda = (y.x() + mu * b.x() - x.x()) / a.x();

    return 0.0 < mu && mu < 1.0
        && 0.0 < lambda && lambda < 1.0;
}

}

