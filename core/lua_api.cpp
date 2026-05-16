#include "lua_api.hpp"
#include "entity_cache.hpp"

namespace lua_api {
    
    typedef void(__cdecl* _lua_pushboolean)(lua_State* L, int b);
    typedef void(__cdecl* _lua_createtable)(lua_State* L, int narr, int nrec);
    typedef void(__cdecl* _lua_setfield)(lua_State* L, int idx, const char* k);
    typedef void(__cdecl* _lua_pushcclosure)(lua_State* L, int (*fn)(lua_State*), int n);
    typedef void(__cdecl* _lua_setglobal)(lua_State* L, const char* name);
    typedef void(__cdecl* _lua_pushstring)(lua_State* L, const char* s);
    typedef double(__cdecl* _lua_tonumber)(lua_State* L, int idx);

    typedef void(__cdecl* _lua_pushnumber)(lua_State* L, double n);
    
    _lua_pushboolean lua_pushboolean = nullptr;
    _lua_createtable lua_createtable = nullptr;
    _lua_setfield lua_setfield = nullptr;
    _lua_pushcclosure lua_pushcclosure = nullptr;
    _lua_setglobal lua_setglobal = nullptr;
    _lua_pushstring lua_pushstring = nullptr;
    _lua_tonumber lua_tonumber = nullptr;
    _lua_pushnumber lua_pushnumber = nullptr;

    int mth_is_friend(lua_State* L) {
        int idx = (int)lua_tonumber(L, 1);
        lua_pushboolean(L, config::friends_list[idx]);
        return 1;
    }

    int mth_is_visible(lua_State* L) {
        // Needs C_BaseEntity arg, dummy for now
        lua_pushboolean(L, false);
        return 1;
    }

    int mth_get_crosshair_ent(lua_State* L) {
        // Return nil for now
        return 0;
    }

    int mth_get_screen_size(lua_State* L) {
        int w = 0, h = 0;
        if (interfaces::engine) interfaces::engine->GetScreenSize(w, h);
        lua_pushnumber(L, w);
        lua_pushnumber(L, h);
        return 2;
    }

    int mth_is_in_game(lua_State* L) {
        bool in_game = interfaces::engine && interfaces::engine->IsInGame();
        lua_pushboolean(L, in_game);
        return 1;
    }

    int mth_get_local_player_name(lua_State* L) {
        if (!interfaces::engine || !interfaces::engine->IsInGame()) {
            lua_pushstring(L, "None");
            return 1;
        }
        player_info_t info;
        if (interfaces::engine->GetPlayerInfo(interfaces::engine->GetLocalPlayer(), &info)) {
            lua_pushstring(L, info.name);
        } else {
            lua_pushstring(L, "Unknown");
        }
        return 1;
    }

    int mth_notify(lua_State* L) {
        return 0;
    }

    int mth_run_lua(lua_State* L) {
        const char* script = lualoader::lua_tolstring(L, 1, nullptr);
        if (script) {
            lualoader::Execute(script);
        }
        return 0;
    }

    int mth_is_menu_open(lua_State* L) {
        lua_pushboolean(L, d3d9hook::menuOpened);
        return 1;
    }

    int mth_get_entity_by_class(lua_State* L) {
        return 0;
    }

    int mth_draw_line(lua_State* L) {
        return 0;
    }

    int mth_draw_box(lua_State* L) {
        return 0;
    }

    int mth_draw_text(lua_State* L) {
        return 0;
    }

    int mth_set_cache(lua_State* L) {
        int index = (int)lua_tonumber(L, 1);
        int is_player = (int)lua_tonumber(L, 2);
        
        std::unique_lock<std::shared_mutex> lock(entity_cache::data_mutex);
        if (is_player) {
            const char* rank = lualoader::lua_tolstring(L, 3, nullptr);
            if (rank) {
                entity_cache::player_ranks[index] = rank;
            }
        } else {
            const char* cls = lualoader::lua_tolstring(L, 3, nullptr);
            if (cls) {
                entity_cache::entity_classes[index] = cls;
            }
        }
        return 0;
    }

    void init(lua_State* L) noexcept {
        if (!L) return;

        HMODULE hLua = GetModuleHandleA("lua_shared.dll");
        if (!hLua) return;

        lua_pushboolean = (_lua_pushboolean)GetProcAddress(hLua, "lua_pushboolean");
        lua_createtable = (_lua_createtable)GetProcAddress(hLua, "lua_createtable");
        lua_setfield = (_lua_setfield)GetProcAddress(hLua, "lua_setfield");
        lua_pushcclosure = (_lua_pushcclosure)GetProcAddress(hLua, "lua_pushcclosure");
        lua_setglobal = (_lua_setglobal)GetProcAddress(hLua, "lua_setglobal");
        lua_pushstring = (_lua_pushstring)GetProcAddress(hLua, "lua_pushstring");
        lua_tonumber = (_lua_tonumber)GetProcAddress(hLua, "lua_tonumber");
        lua_pushnumber = (_lua_pushnumber)GetProcAddress(hLua, "lua_pushnumber");

        lua_createtable(L, 0, 15);
        
        #define BIND_API(name) lua_pushcclosure(L, mth_##name, 0); lua_setfield(L, -2, #name);
        
        BIND_API(is_friend)
        BIND_API(is_visible)
        BIND_API(get_crosshair_ent)
        BIND_API(get_screen_size)
        BIND_API(is_in_game)
        BIND_API(get_local_player_name)
        BIND_API(set_cache)
        BIND_API(notify)
        BIND_API(run_lua)
        BIND_API(is_menu_open)
        BIND_API(get_entity_by_class)
        BIND_API(draw_line)
        BIND_API(draw_box)
        BIND_API(draw_text)

        #undef BIND_API

        lua_setglobal(L, "mth");
    }

    bool run_script(lua_State* L, const std::string& script, const std::string& name) noexcept {
        if (!L) return false;
        if (lualoader::luaL_loadbuffer(L, script.c_str(), script.size(), name.c_str()) != 0) return false;
        if (lualoader::lua_pcall(L, 0, 0, 0) != 0) return false;
        return true;
    }
}