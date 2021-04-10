#pragma once

#include <unordered_set>

namespace input {

class KeyState {
   public:
    KeyState();

    bool isPressed(int key) const;
    bool isPosEdge(int key) const;
    bool isNegEdge(int key) const;

    void updateState(int key, bool pressed);
    void step();

   private:
    std::unordered_set<int> _pressed;
    std::unordered_set<int> _posEdge;
    std::unordered_set<int> _negEdge;
};

}  // namespace input
