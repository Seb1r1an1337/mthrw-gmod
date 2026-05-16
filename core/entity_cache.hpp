#pragma once
#include "../includes.hpp"
#include <shared_mutex>
#include <vector>

struct CachedEntity {
    C_BasePlayer* entity;
    int index;
    bool is_player;
    bool is_alive;
    bool is_dormant;
    int health;
    int max_health;
    int armor;
    Vector origin;
    player_info_t player_info;
    const char* network_name;
    const char* model_name;
    int class_id;
};

namespace entity_cache {
    extern std::vector<CachedEntity> players;
    extern std::vector<CachedEntity> npcs;
    extern std::vector<CachedEntity> weapons;
    extern std::vector<CachedEntity> entities;

    extern std::shared_mutex cache_mutex;

    extern std::map<int, std::string> player_ranks;
    extern std::map<int, std::string> player_names;
    extern std::map<int, std::string> player_jobs;
    extern std::map<int, ImColor> player_team_colors;
    extern std::map<int, int> player_health;
    extern std::map<int, int> player_max_health;
    extern std::map<int, int> player_armor;
    extern std::map<int, std::string> entity_classes;
    extern std::shared_mutex data_mutex;

    void update();
    void update_lua_data();
}
