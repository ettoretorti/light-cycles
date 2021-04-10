#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <mathfu/glsl_mappings.h>

#include "Cycle.hpp"
#include "Trail.hpp"

namespace lcycle {

struct Player {
    using Color = mathfu::vec4;

    Cycle cycle;
    int id;
    std::string name;
    Color color;
    Color tColor;
};

class World {
   public:
    using PlayerInputs = std::vector<std::pair<int, CycleInput>>;

    World(double size, double dashTime, const std::vector<Player>& players);
    World();

    World(const World& other) = default;
    World& operator=(const World& other) = default;
    World(World&& other) = default;
    World& operator=(World&& other) = default;

    void runFor(double secs, const PlayerInputs& inputs);
    const std::vector<Player>& players() const;
    const std::vector<Trail>& trails() const;
    double size() const;

   private:
    std::vector<Player> _players;
    std::vector<Trail> _trails;
    double _size;
    double _dashTime;
    double _curTime;
    bool _drawing;
};

}  // namespace lcycle
