#pragma once

#include <mathfu/glsl_mappings.h>


namespace lcycle {

class Line {
public:
    Line(const mathfu::vec2& start, const mathfu::vec2& end);
    Line(const Line& other);
    Line& operator=(const Line& other);
    Line(Line&& other);
    Line& operator=(Line&& other);

    const mathfu::vec2& start() const;
    const mathfu::vec2& end() const;
    mathfu::vec2& start();
    mathfu::vec2& end();

    double len() const;
    double lenSquared() const;

    static bool intersect(const Line& a, const Line& b);
private:
    mathfu::vec2 _s, _e;
};

}
