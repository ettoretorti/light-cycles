#pragma once

#include "lcycle/World.hpp"
#include "util/CircularBuffer.hpp"

namespace lcycle {

class RollbackWorld {
   public:
    RollbackWorld(const World& w);
    RollbackWorld();

    World* latest();
    bool rollback(int frames);
    void advance(const World::PlayerInputs& inputs);

   private:
    util::CircularBuffer<World, 64> _buf;
};

}  // namespace lcycle
