//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once
#include <string>
#include <thread>

#include "portal/core/common.h"

namespace portal
{

class Thread
{
public:
    Thread(const std::string& name);

    template<typename Fn, typename ...Args>
    void dispatch(Fn&& func, Args&&... args)
    {
        thread = std::thread(func, std::forward<Args>(args)...);
        set_name(name);
    }

    void set_name(const std::string& in_name);
    void join();

    std::thread::id get_id() const;
private:
    std::string name;
    std::thread thread;
};


class ThreadSignal
{
public:
    ThreadSignal(const std::string& name, bool manual_reset = false);

    void wait() const;
    void signal() const;
    void reset() const;
private:
    void* signal_handle = nullptr;
};


void set_main_thread(std::thread::id id);
bool is_main_thread();
}
