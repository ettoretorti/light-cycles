#pragma once

namespace util {

template <typename T, int N>
class CircularBuffer final {
public:
    CircularBuffer(): _buf(),_head(0), _size(0) {}
    ~CircularBuffer() {}
    T* head() { return _size > 0 ? &_buf[_head] : nullptr; }
    T* tail() { return _size > 0 ? &_buf[(_head + _size - 1) % N] : nullptr; }
    T* add() {
        if(_size < N) {
            _size++;
            return tail();
        } else {
            T* t = &_buf[_head];
            _head = (_head + 1) % N;
            return t;
        }
    }
    bool remove() {
        if(_size > 0) {
            _size--;
            return true;
        }
        return false;
    }
    int size() {
        return _size;
    }

private:
    T _buf[N];
    int _head;
    int _size;
};

}
