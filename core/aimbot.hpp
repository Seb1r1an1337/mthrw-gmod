#pragma once
#include "../includes.hpp"
#include "entity_cache.hpp"

namespace aimbot {
    void run(CUserCmd* cmd) noexcept;
    
    // Internal helpers
    bool IsVisible(const Vector& start, const Vector& end, C_BasePlayer* target) noexcept;
}
