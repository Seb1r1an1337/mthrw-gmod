#pragma once
#include "../includes.hpp"

namespace lua_api {
    void init(lua_State* L) noexcept;
    bool run_script(lua_State* L, const std::string& script, const std::string& name = "@lua/includes/init.lua") noexcept;
}