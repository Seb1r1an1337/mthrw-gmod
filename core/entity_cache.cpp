#include "entity_cache.hpp"
#include "visuals.hpp"

namespace entity_cache {
    std::vector<CachedEntity> players;
    std::vector<CachedEntity> npcs;
    std::vector<CachedEntity> weapons;
    std::vector<CachedEntity> entities;
    std::shared_mutex cache_mutex;

    std::map<int, std::string> player_ranks;
    std::map<int, std::string> player_names;
    std::map<int, std::string> player_jobs;
    std::map<int, ImColor> player_team_colors;
    std::map<int, int> player_health;
    std::map<int, int> player_max_health;
    std::map<int, int> player_armor;
    std::map<int, std::string> entity_classes;
    std::shared_mutex data_mutex;

    struct LuaAutoPop {
        CLuaInterface* lua;
        int top;
        LuaAutoPop(CLuaInterface* l) : lua(l) { top = lua->Top(); }
        ~LuaAutoPop() { 
            int current = lua->Top();
            if (current > top)
                lua->Pop(current - top); 
        }
    };

    struct RawEntityData {
        bool is_dormant;
        Vector origin;
        const char* network_name;
        int class_id;
    };

    bool SEH_ReadEntity(C_BasePlayer* ent, RawEntityData* out) {
        __try {
            out->is_dormant = ent->IsDormant();
            out->origin = ent->GetAbsOrigin();
            ClientClass* cc = ent->GetClientClass();
            if (cc && !IsBadReadPtr(cc, sizeof(ClientClass)) && cc->m_pNetworkName && !IsBadReadPtr((void*)cc->m_pNetworkName, 1)) {
                out->network_name = cc->m_pNetworkName;
                out->class_id = cc->m_ClassID;
            } else {
                out->network_name = "Unknown";
                out->class_id = -1;
            }
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    bool SEH_ReadAlive(C_BasePlayer* ent, bool* alive, int* hp, int* max_hp) {
        __try {
            *alive = ent->IsAlive();
            *hp = ent->GetHealth();
            *max_hp = ent->GetMaxHealth();
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    bool SEH_FetchLua(CLuaInterface* lua, int index, char* name_out, size_t max_name_len, char* class_out, size_t max_class_len, char* group_out, size_t max_group_len, char* job_out, size_t max_job_len, int* health_out, int* armor_out, int* max_health_out, int* r_out, int* g_out, int* b_out) {
        int top = lua->Top();
        bool success = false;
        *r_out = 255; *g_out = 255; *b_out = 255; // Default white
        __try {
            lua->PushSpecial(0); // Global table
            lua->GetField(-1, "Entity");
            if (lua->IsType(-1, 6)) { // Entity function
                lua->PushNumber(index);
                lua->Call(1, 1);
                if (lua->GetType(-1) > 0) { // Should be 9 (Entity) or userdata
                    int ent_idx = lua->Top();

                    // 0. Class (Crucial for scripted entities like edit_fog)
                    lua->GetField(ent_idx, "GetClass");
                    if (lua->IsType(-1, 6)) {
                        lua->Push(ent_idx); lua->Call(1, 1);
                        if (lua->IsType(-1, 4)) {
                            const char* res = lua->GetString(-1, NULL);
                            if (res) strncpy_s(class_out, max_class_len, res, _TRUNCATE);
                        }
                        lua->Pop(1);
                    } else lua->Pop(1);

                    // Name
                    lua->GetField(ent_idx, "Nick");
                    if (lua->IsType(-1, 6)) {
                        lua->Push(ent_idx); lua->Call(1, 1);
                        if (lua->IsType(-1, 4)) strncpy_s(name_out, max_name_len, lua->GetString(-1, NULL), _TRUNCATE);
                        lua->Pop(1);
                    } else lua->Pop(1);

                    // UserGroup
                    lua->GetField(ent_idx, "GetUserGroup");
                    if (lua->IsType(-1, 6)) {
                        lua->Push(ent_idx); lua->Call(1, 1);
                        if (lua->IsType(-1, 4)) {
                            const char* res = lua->GetString(-1, NULL);
                            if (res) strncpy_s(group_out, max_group_len, res, _TRUNCATE);
                        }
                        lua->Pop(1);
                    } else lua->Pop(1);

                    // Job / Team
                    bool job_found = false;
                    int team_id = 0;
                    lua->GetField(ent_idx, "Team");
                    if (lua->IsType(-1, 6)) {
                        lua->Push(ent_idx); lua->Call(1, 1);
                        team_id = (int)lua->GetNumber(-1);
                        lua->Pop(1);
                    } else lua->Pop(1);

                    lua->GetField(ent_idx, "getDarkRPVar");
                    if (lua->IsType(-1, 6)) {
                        lua->Push(ent_idx); lua->PushString("job"); lua->Call(2, 1);
                        if (lua->IsType(-1, 4)) {
                            const char* res = lua->GetString(-1, NULL);
                            if (res && res[0] != '\0') {
                                strncpy_s(job_out, max_job_len, res, _TRUNCATE);
                                job_found = true;
                            }
                        }
                        lua->Pop(1);
                    } else lua->Pop(1);

                    // Team Name Fallback & Color
                    lua->PushSpecial(0); lua->GetField(-1, "team");
                    if (lua->IsType(-1, 5)) {
                        // Color
                        lua->GetField(-1, "GetColor");
                        if (lua->IsType(-1, 6)) {
                            lua->PushNumber(team_id); lua->Call(1, 1);
                            if (lua->IsType(-1, 5)) {
                                lua->GetField(-1, "r"); *r_out = (int)lua->GetNumber(-1); lua->Pop(1);
                                lua->GetField(-1, "g"); *g_out = (int)lua->GetNumber(-1); lua->Pop(1);
                                lua->GetField(-1, "b"); *b_out = (int)lua->GetNumber(-1); lua->Pop(1);
                            }
                            lua->Pop(1);
                        } else lua->Pop(1);

                        // Name fallback
                        if (!job_found) {
                            lua->GetField(-1, "GetName");
                            if (lua->IsType(-1, 6)) {
                                lua->PushNumber(team_id); lua->Call(1, 1);
                                if (lua->IsType(-1, 4)) strncpy_s(job_out, max_job_len, lua->GetString(-1, NULL), _TRUNCATE);
                                lua->Pop(1);
                            } else lua->Pop(1);
                        }
                    }
                    lua->Pop(2); // team table + GLOB

                    // Stats
                    lua->GetField(ent_idx, "Health"); if (lua->IsType(-1, 6)) { lua->Push(ent_idx); lua->Call(1, 1); *health_out = (int)lua->GetNumber(-1); lua->Pop(1); } else lua->Pop(1);
                    lua->GetField(ent_idx, "Armor"); if (lua->IsType(-1, 6)) { lua->Push(ent_idx); lua->Call(1, 1); *armor_out = (int)lua->GetNumber(-1); lua->Pop(1); } else lua->Pop(1);
                    
                    success = true;
                }
            }
            lua->Pop(lua->Top() - top);
            return success;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            if (lua) lua->Pop(lua->Top() - top);
            return false;
        }
    }

    bool SafeReadEntity(C_BasePlayer* ent, CachedEntity& out_cached) {
        RawEntityData data;
        if (SEH_ReadEntity(ent, &data)) {
            out_cached.is_dormant = data.is_dormant;
            out_cached.origin = data.origin;
            out_cached.network_name = data.network_name;
            out_cached.class_id = data.class_id;
            return true;
        }
        return false;
    }
    
    bool SafeReadPlayerStatus(C_BasePlayer* ent, CachedEntity& out_cached) {
        bool alive = false; int hp = 0, mhp = 100;
        if (SEH_ReadAlive(ent, &alive, &hp, &mhp)) {
            out_cached.is_alive = alive;
            std::shared_lock<std::shared_mutex> lock(data_mutex);
            if (player_health.count(out_cached.index)) out_cached.health = player_health[out_cached.index]; else out_cached.health = hp;
            if (player_max_health.count(out_cached.index)) out_cached.max_health = player_max_health[out_cached.index]; else out_cached.max_health = mhp;
            if (player_armor.count(out_cached.index)) out_cached.armor = player_armor[out_cached.index]; else out_cached.armor = 0;
            return true;
        }
        return false;
    }

    bool SafeFetchLuaData(CLuaInterface* lua, int index, char* name_out, size_t max_name_len, char* class_out, size_t max_class_len, char* group_out, size_t max_group_len, char* job_out, size_t max_job_len, int& health_out, int& armor_out, int& max_health_out, ImColor& team_col) {
        int hp = 0, arm = 0, mhp = 100, r = 255, g = 255, b = 255;
        if (SEH_FetchLua(lua, index, name_out, max_name_len, class_out, max_class_len, group_out, max_group_len, job_out, max_job_len, &hp, &arm, &mhp, &r, &g, &b)) {
            health_out = hp; armor_out = arm; max_health_out = mhp; 
            team_col = ImColor(r, g, b, 255);
            return true;
        }
        return false;
    }

    void update() {
        if (!interfaces::engine->IsInGame() || !interfaces::engine->IsConnected()) return;
        int local_index = interfaces::engine->GetLocalPlayer();
        std::vector<CachedEntity> new_players, new_npcs, new_weapons, new_entities;
        int highest_index = interfaces::entityList->GetHighestEntityIndex();
        
        static std::map<int, int> old_health_map;

        for (int i = 0; i <= highest_index; ++i) {
            C_BasePlayer* ent = (C_BasePlayer*)interfaces::entityList->GetClientEntity(i);        
            if (!ent) {
                old_health_map[i] = 0;
                continue;
            }

            if (i == 0) continue; 
            
            CachedEntity cached; cached.entity = ent; cached.index = i;
            if (!SafeReadEntity(ent, cached)) { 
                cached.is_dormant = true; 
                cached.origin = Vector(0,0,0); 
                cached.network_name = "Unknown"; 
                cached.class_id = -1; 
            }
            
            cached.model_name = "Unknown";
            
            if (interfaces::engine->GetPlayerInfo(i, &cached.player_info)) {
                if (i == local_index) {
                    old_health_map[i] = ent->GetHealth();
                    continue;
                }
                cached.is_player = true;
                if (!SafeReadPlayerStatus(ent, cached)) { cached.is_alive = false; cached.health = 0; cached.max_health = 100; cached.armor = 0; }
                
                old_health_map[i] = cached.health;
                new_players.push_back(cached);
            } else {
                cached.is_player = false;
                std::string name(cached.network_name);
                if (name.empty() || name == "worldspawn" || name == "class C_BaseEntity") continue;

                if (name.find("Weapon") != std::string::npos || name.find("CWeapon") != std::string::npos || name.find("weapon_") != std::string::npos) {
                    new_weapons.push_back(cached);
                } else if (name.find("NPC") != std::string::npos || name.find("npc_") != std::string::npos) {
                    new_npcs.push_back(cached);
                } else {
                    new_entities.push_back(cached);
                }
            }
        }
        { std::unique_lock<std::shared_mutex> lock(cache_mutex); players = std::move(new_players); npcs = std::move(new_npcs); weapons = std::move(new_weapons); entities = std::move(new_entities); }
    }

    void update_lua_data() {
        if (!interfaces::engine->IsInGame() || !interfaces::engine->IsConnected()) return;
        static float last_update_time = 0.f;
        float curtime = interfaces::engine->Time();
        if (curtime - last_update_time > 0.1f || curtime < last_update_time) {    
            last_update_time = curtime;
            if (interfaces::cluaShared && !interfaces::engine->IsDrawingLoadingImage()) {
                CLuaInterface* lua = interfaces::cluaShared->GetLuaInterface(LUAINTERFACE_CLIENT);
                if (lua) {
                    std::map<int, std::string> new_ranks, new_classes, new_names, new_jobs;
                    std::map<int, int> new_health, new_max_health, new_armor;
                    std::map<int, ImColor> new_team_colors;
                    int highest_index = interfaces::entityList->GetHighestEntityIndex();
                    for (int i = 1; i <= highest_index; ++i) {
                        void* ent = interfaces::entityList->GetClientEntity(i); if (!ent) continue;
                        char class_buf[128] = {0}, group_buf[128] = {0}, name_buf[128] = {0}, job_buf[128] = {0};
                        int hp = 0, arm = 0, mhp = 100;
                        ImColor team_col;
                        if (SafeFetchLuaData(lua, i, name_buf, sizeof(name_buf), class_buf, sizeof(class_buf), group_buf, sizeof(group_buf), job_buf, sizeof(job_buf), hp, arm, mhp, team_col)) {
                            if (class_buf[0] != '\0') new_classes[i] = std::string(class_buf);
                            if (group_buf[0] != '\0') new_ranks[i] = std::string(group_buf);
                            if (name_buf[0] != '\0') new_names[i] = std::string(name_buf);
                            if (job_buf[0] != '\0') new_jobs[i] = std::string(job_buf);
                            new_health[i] = hp; new_max_health[i] = mhp; new_armor[i] = arm;
                            new_team_colors[i] = team_col;
                        }
                    }
                    std::unique_lock<std::shared_mutex> data_lock(data_mutex);    
                    player_ranks = std::move(new_ranks); entity_classes = std::move(new_classes);
                    player_names = std::move(new_names); player_jobs = std::move(new_jobs);
                    player_health = std::move(new_health); player_max_health = std::move(new_max_health);
                    player_armor = std::move(new_armor);
                    player_team_colors = std::move(new_team_colors);
                }
            }
        }
    }
}
