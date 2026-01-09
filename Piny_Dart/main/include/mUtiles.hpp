#ifndef MUTILES_HPP
#define MUTILES_HPP

#pragma once

#include <thread>
#include <mutex>
#include <iostream>

namespace mUtiles
{
    void sleep_ms(int ms);

    static int set_thread_priority(std::thread &thread, int policy, int priority);

} // namespace mUtiles
#endif /* MUTILES_HPP */