#include "lcycle/RollbackWorld.hpp"

#include "util/CircularBuffer.hpp"
#include "lcycle/World.hpp"


namespace lcycle {

RollbackWorld::RollbackWorld(const World& w): _buf() {
    for(int i = 0; i < 64; i++) {
        *_buf.add() = w;
    }
}

RollbackWorld::RollbackWorld(): RollbackWorld::RollbackWorld(World()) {}

World* RollbackWorld::latest() { return _buf.tail(); }

bool RollbackWorld::rollback(int frames) {
    if(_buf.size() > frames) {
        for(int i = 0; i < frames; i++) {
            _buf.remove();
        }
        return true;
    }
    return false;
}

void RollbackWorld::advance(const World::PlayerInputs& inputs) {
    constexpr double duration = 1.0/60.0;
    World* cur = _buf.tail();
    World* next = _buf.add();
    *next = *cur;
    next->runFor(duration, inputs);
}

}
