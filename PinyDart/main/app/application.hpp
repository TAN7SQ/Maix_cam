#pragma once

#include "_basic.hpp"

class App
{
public:
    App() = default;
    ~App() = default;

    void appSchedule();

private:
    void appInit();
};