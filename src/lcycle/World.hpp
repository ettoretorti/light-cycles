#pragma once

#include <vector>
#include <functional>

#include <mathfu/glsl_mappings.h>

#include "Cycle.hpp"
#include "Trail.hpp"


namespace lcycle {

struct Player {
        using Color = mathfu::vec4;

        Cycle cycle;
        std::function<CycleInput()> input;
        Color color;
        Color tColor;
};


class World {

public:
    World(double size, double dashTime, std::vector<Player>&& players);
    
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other);
    World& operator=(World&& other);
    
    void runFor(double secs);
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

}
