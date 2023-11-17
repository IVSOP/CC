#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <stdint.h>
#include <mutex>
#include <condition_variable>

// this class does NOT allocate any memory on the heap, do it yourself

// pop when empty: blocks
// push when full: blocks

// alternativa a usar 2 conditions: usar atomic int
// ver https://fabiensanglard.net/doom3_bfg/threading.php

template<typename T, size_t len>
class BoundedBuffer {
public:
    BoundedBuffer() : size(0), mutex(), notFull(), notEmpty() {}

    ~BoundedBuffer() = default;

    void push(const T &item) {
        std::unique_lock<std::mutex> lock(mutex); // lock fica aqui e so a mutex e que e comum a todos?????????????????

        while (size >= len) { // full
            notFull.wait(lock); // em vez de while loop podemos passar lambda function
        }

        data[size] = item;
        size++;

        notEmpty.notify_one();
        lock.unlock();
    }

    T pop() { // ineficiente porque copia memoria, mas fica organizado. em principio nunca vai ser muita memoria tbm
        std::unique_lock<std::mutex> lock(mutex); // lock fica aqui e so a mutex e que e comum a todos?????????????????

        while (size < 1) { // empty
            notEmpty.wait(lock); // em vez de while loop podemos passar lambda function
        }

        size--;
        const size_t _size = size;

        notFull.notify_one();
        lock.unlock();

        return data[_size]; // ja levou --
    }

private:
    T data[len];
    size_t size;
    std::mutex mutex;

    std::condition_variable notFull;
    std::condition_variable notEmpty;
};

#endif
