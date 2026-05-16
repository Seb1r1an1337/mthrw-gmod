#pragma once
#include "../includes.hpp"
#include "entity_cache.hpp"
#include <mutex>
#include <queue>
#include <string>

namespace visuals {
    extern std::queue<std::string> chat_queue;
    extern std::mutex chat_mutex;
    void run();
}
