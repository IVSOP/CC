#ifndef TP2_THREADRAII_H
#define TP2_THREADRAII_H

/**
 * Code from:
 * https://github.com/ordinary-developer/book_effective_modern_c_plus_plus_s_meyers/blob/60e5605060959b8c49b70da4addc9696e45dcd2b/code/ch_7-THE_CONCURRENCY_API/item_37-make_std_threads_unjoinable_on_all_paths/02-ThreadRAII/main.cpp
 * Adicional Links:
 * https://github.com/luuvish/effective-modern-cpp/blob/3c8902b2e8982f29a87ba4dcc444a0ef778307bf/ch7-item37.cc
 * */

#include <iostream>
#include <thread>
#include <memory>

class ThreadRAII {
public:
    enum class DtorAction {
        join, detach
    };

    ThreadRAII(std::thread &&t, DtorAction a) : action(a), t(std::move(t)) {}

    ~ThreadRAII() {
        if (t.joinable()) {
            if (action == DtorAction::join) {
                t.join();
            } else t.detach();
        }
    }

    ThreadRAII(ThreadRAII &&) = default;

    ThreadRAII &operator=(ThreadRAII &&) = default;

    std::thread &get() { return t; }

private:
    DtorAction action;
    std::thread t;
};

#endif //TP2_THREADRAII_H
