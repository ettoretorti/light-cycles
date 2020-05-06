#pragma once

#include "util/CircularBuffer.hpp"
#include "lcycle/World.hpp"


namespace lcycle {

class RollbackWorld {
public:
    RollbackWorld(const World& w);

    World* latest();
    bool rollback(int frames);
    void advance(const World::PlayerInputs& inputs);

private:
    util::CircularBuffer<World, 64> _buf;
};

}
