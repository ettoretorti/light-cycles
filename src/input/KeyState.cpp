#include "KeyState.hpp"

#include <unordered_set>

namespace input {

KeyState::KeyState() : _pressed(), _posEdge(), _negEdge() {}

bool KeyState::isPressed(int key) const {
    return _pressed.count(key) > 0;
}

bool KeyState::isPosEdge(int key) const {
    return _posEdge.count(key) > 0;
}

bool KeyState::isNegEdge(int key) const {
    return _negEdge.count(key) > 0;
}

void KeyState::updateState(int key, bool pressed) {
    bool prev = isPressed(key);
    if (!prev && pressed) {
        _posEdge.insert(key);
        _pressed.insert(key);
    } else if (prev && !pressed) {
        _negEdge.insert(key);
        _pressed.erase(key);
    }
}

void KeyState::step() {
    _posEdge.clear();
    _negEdge.clear();
}

} // namespace input
