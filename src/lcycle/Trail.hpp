#pragma once

#include <mathfu/constants.h>
#include <mathfu/glsl_mappings.h>
#include <vector>
#include "Line.hpp"

namespace lcycle {

class Trail {
   public:
    Trail(const mathfu::vec4& color = mathfu::kOnes4f);
    Trail(const Trail& other);
    Trail& operator=(const Trail& other);
    Trail(Trail&& other);
    Trail& operator=(Trail&& other);

    size_t size() const;
    Line& operator[](size_t idx);
    const Line& operator[](size_t idx) const;
    const std::vector<Line>& data() const;

    const mathfu::vec4& color() const;
    mathfu::vec4& color();

    void add(const Line& line);

   private:
    std::vector<Line> _trail;
    mathfu::vec4 _color;
};

}  // namespace lcycle
