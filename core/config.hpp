#pragma once
#include "../includes.hpp"
#include <map>
#include <string>

namespace config {
    // CORE
    inline float currentvelocity = 0.f;
    inline lua_State* luastate = nullptr;
    inline bool allowcslua = false;
    inline bool allowcheats = false;

    // AIM / COMBAT
    inline bool aim_enabled = false;
    inline bool no_spread_bool = false;
    inline bool no_recoil_bool = false;
    inline bool silent_aim = false;
    inline bool visible_aim = true;
    inline int aim_mode = 0; 
    inline int target_selection = 0; 
    inline float fov = 10.f;
    inline bool draw_fov = true;
    inline float fov_color[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
    inline bool target_lock = false;
    inline bool auto_fire = false;
    inline bool auto_wall = false;
    inline bool shoot_on_lock = false;
    inline bool engine_prediction = false;
    inline bool velocity_extrapolation = false;
    inline int hitbox_priority = 0; 
    inline bool hitbox_override = false;
    inline bool backtrack = false;
    inline int backtrack_ticks = 12;
    inline int resolver = 0; 
    inline int no_spread = 0; 
    inline int no_recoil = 0; 
    inline float recoil_control = 100.f;
    inline float smoothing = 0.f;
    inline bool humanization = false;
    inline bool triggerbot = false;
    inline int trigger_delay = 0;
    inline bool trigger_visible = true;

    inline bool ignore_friends = true;
    inline bool ignore_team = false;
    inline bool ignore_dormant = true;
    inline bool ignore_invisible = true;
    inline bool ignore_transparent = false;
    inline bool ignore_nocollide = true;
    inline bool ignore_npcs = true;
    inline bool aim_players = true;
    inline bool aim_npcs = false;
    inline bool aim_bots = true;
    inline bool aim_props = false;
    inline bool aim_vehicles = false;
    inline bool aim_ragdolls = false;

    struct ESPComponent {
        bool enabled = false;
        int side = 0; // 0: Top, 1: Bottom, 2: Left, 3: Right
        int order = 0;
        float padding = 2.0f;
        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool visible_only = false;
    };

    inline bool esp_enabled = true;
    inline int esp_box_type = 0; // 0: Flat, 1: Bounding, 2: Corners, 3: 3D
    inline bool esp_color_by_team = false;

    inline bool esp_cat_players = true;
    inline bool esp_cat_npcs = false;
    inline bool esp_cat_weapons = false;
    inline bool esp_cat_props = false;
    inline bool esp_cat_ragdolls = false;
    inline bool esp_cat_darkrp = false;
    inline bool esp_cat_vehicles = false;
    inline bool esp_cat_world = false;

    inline bool esp_players_visible_only = false;
    inline bool esp_entities_visible_only = false;
    inline float esp_max_dist_players = 1000.f;
    inline float esp_max_dist_entities = 1000.f;
    inline bool esp_vis_check = true;
    inline bool esp_dormant = false;
    inline int radar_style = 0; 
    inline float radar_zoom = 1.0f;

    inline ESPComponent comp_name = { true, 0, 0, 2.0f, {1, 1, 1, 1}, false };
    inline ESPComponent comp_health_pct = { true, 2, 0, 2.0f, {1, 1, 1, 1}, false };
    inline ESPComponent comp_health_bar = { true, 2, 1, 4.0f, {0, 1, 0, 1}, false };
    inline ESPComponent comp_armor_bar = { true, 1, 0, 2.0f, {0, 0.5f, 1, 1}, false };
    inline ESPComponent comp_weapon = { true, 1, 0, 2.0f, {1, 1, 1, 1}, false };
    inline ESPComponent comp_user_group = { true, 3, 0, 2.0f, {1, 1, 0, 1}, false };
    inline ESPComponent comp_job = { true, 3, 1, 2.0f, {0, 1, 0, 1}, false };
    inline ESPComponent comp_team = { false, 3, 2, 2.0f, {1, 1, 1, 1}, false };
    inline ESPComponent comp_distance = { true, 1, 1, 2.0f, {0.8f, 0.8f, 0.8f, 1}, false };
    inline ESPComponent comp_snapline = { false, 1, 0, 0.0f, {1, 0, 0, 1}, false };
    inline ESPComponent comp_skeleton = { false, 0, 0, 0.0f, {1, 1, 1, 1}, false };
    inline ESPComponent comp_headdot = { false, 0, 0, 0.0f, {1, 0, 0, 1}, false };
    inline ESPComponent comp_box3d = { false, 0, 0, 0.0f, {0, 1, 0.6f, 1}, false };
    inline ESPComponent comp_box2d = { true, 0, 0, 0.0f, {0, 1, 0.6f, 1}, false };
    inline ESPComponent comp_boxcorner = { false, 0, 0, 0.0f, {0, 1, 0.6f, 1}, false };
    inline ESPComponent comp_glow = { false, 0, 0, 0.0f, {1, 0, 0, 1}, false };
    inline ESPComponent comp_flags = { false, 3, 1, 2.0f, {1, 1, 1, 1}, false };
    inline ESPComponent comp_offscreen = { false, 0, 0, 0.0f, {1, 0, 0, 1}, false };

    inline bool chams_enabled = true;
    inline int chams_mat = 0; 
    inline bool custom_crosshair = false;
    inline bool recoil_crosshair = false;
    inline bool hitmarker = false;
    inline bool damage_logs = false;
    inline bool killfeed = false;
    inline bool hand_chams = false;
    inline bool grenade_prediction = false;

    inline bool bunnyhop = true;
    inline int bhop_mode = 0; // 0: Standard, 1: Safe
    inline int max_hops = 0;
    inline bool autostrafe = true;
    inline int autostrafe_mode = 0; // 0: Legit, 1: Rage
    inline bool anti_screenshot = true;
    inline char lua_chunkname[32] = "lua_shared";

    inline bool aa_enabled = false;
    inline int aa_pitch = 0; 
    inline int aa_yaw = 0; 

    inline bool wnd_environment_list = false;
    inline bool wnd_lua_executor = false;
    inline bool wnd_spectators = false;
    inline bool wnd_console = false;
    inline bool wnd_chat_spam = false;

    inline std::vector<std::string> chat_spam_list = { "i love methamphetamine.solutions" };
    inline std::vector<std::string> kill_say_list = { "powned by methamphetamine.solutions" };
    inline bool chat_spam_save_on_mod = true;
    inline bool chat_spam_ooc = true;
    inline float chat_spam_delay = 3.0f;
    inline bool chat_spam_enabled = false;
    inline bool kill_say_enabled = false;

    inline std::map<int, bool> friends_list;
    inline std::map<int, bool> priority_list;
    
    struct CustomESPComponent {
        bool enabled = false;
        int side = 3; // Right by default
        float padding = 0.f;
        float color[4] = { 1.f, 1.f, 1.f, 1.f };
    };

    struct CustomEntityESP {
        bool enabled = false;
        CustomESPComponent box2d;
        CustomESPComponent box3d;
        CustomESPComponent name;
        CustomESPComponent distance;
    };
    inline std::map<std::string, CustomEntityESP> custom_entity_esp;
}
