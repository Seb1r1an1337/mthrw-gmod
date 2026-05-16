#include "visuals.hpp"
#include <algorithm>
#include <string>
#include <chrono>

#undef min
#undef max

namespace visuals {

    VMatrix g_ViewMatrix;
    int g_ScreenWidth = 0;
    int g_ScreenHeight = 0;

    std::queue<std::string> chat_queue;
    std::mutex chat_mutex;

    bool WorldToScreen(const Vector& origin, ImVec2& screen) {
        float w = g_ViewMatrix[3][0] * origin.x + g_ViewMatrix[3][1] * origin.y + g_ViewMatrix[3][2] * origin.z + g_ViewMatrix[3][3];
        if (w < 0.001f) return false;
        float invw = 1.0f / w;
        float x = (origin.x * g_ViewMatrix[0][0] + origin.y * g_ViewMatrix[0][1] + origin.z * g_ViewMatrix[0][2] + g_ViewMatrix[0][3]) * invw;
        float y = (origin.x * g_ViewMatrix[1][0] + origin.y * g_ViewMatrix[1][1] + origin.z * g_ViewMatrix[1][2] + g_ViewMatrix[1][3]) * invw;
        screen.x = (g_ScreenWidth / 2.0f) + (x * g_ScreenWidth) / 2.0f;
        screen.y = (g_ScreenHeight / 2.0f) - (y * g_ScreenHeight) / 2.0f;
        return true;
    }

    struct Box {
        float x, y, w, h;
    };

    bool IsVisible(C_BaseEntity* ent) {
        if (!localPlayer || !ent) return false;
        auto get_eyes = [](C_BaseEntity* e) -> Vector { return e->GetAbsOrigin() + *reinterpret_cast<Vector*>(uintptr_t(e) + 0x108); };
        Vector src = get_eyes(localPlayer);
        Vector target_eyes = get_eyes(ent);
        Vector target_center = ent->GetAbsOrigin() + Vector(0, 0, 35.f);
        Ray_t ray; trace_t tr; TraceFilterSimple filter(localPlayer);
        ray.Init(src, target_eyes);
        interfaces::trace->TraceRay(ray, MASK_SHOT, &filter, (CTrace*)&tr);
        if (tr.m_pEnt == (C_BasePlayer*)ent || tr.fraction >= 0.98f) return true;
        ray.Init(src, target_center);
        interfaces::trace->TraceRay(ray, MASK_SHOT, &filter, (CTrace*)&tr);
        if (tr.m_pEnt == (C_BasePlayer*)ent || tr.fraction >= 0.98f) return true;
        return false;
    }

    void DrawString(ImDrawList* drawList, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, const char* text, bool centered = true) {
        if (!text) return;
        ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);
        ImVec2 final_pos = pos;
        if (centered) final_pos.x -= text_size.x / 2.0f;
        drawList->AddText(font, font_size, ImVec2(final_pos.x - 1, final_pos.y - 1), IM_COL32(0, 0, 0, 255), text);
        drawList->AddText(font, font_size, ImVec2(final_pos.x + 1, final_pos.y - 1), IM_COL32(0, 0, 0, 255), text);
        drawList->AddText(font, font_size, ImVec2(final_pos.x - 1, final_pos.y + 1), IM_COL32(0, 0, 0, 255), text);
        drawList->AddText(font, font_size, ImVec2(final_pos.x + 1, final_pos.y + 1), IM_COL32(0, 0, 0, 255), text);
        drawList->AddText(font, font_size, final_pos, color, text);
    }

    inline float calc_text_size(const Box& box) {
        float size = 14.f;
        if (box.h < 55) { size -= size * (box.h / 100.f); if (size < 11.f) size = 11.f; }
        return size;
    }

    void DrawCornerBox(const Box& box, ImU32 color) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        float line_w = box.w / 4.0f; float line_h = box.h / 4.0f;
        ImU32 black = IM_COL32(0, 0, 0, 255);
        auto draw_corner = [&](ImVec2 p1, ImVec2 p2, ImVec2 p3) { drawList->AddLine(p1, p2, black, 3.0f); drawList->AddLine(p1, p3, black, 3.0f); drawList->AddLine(p1, p2, color, 1.0f); drawList->AddLine(p1, p3, color, 1.0f); };
        draw_corner(ImVec2(box.x, box.y), ImVec2(box.x + line_w, box.y), ImVec2(box.x, box.y + line_h));
        draw_corner(ImVec2(box.x + box.w, box.y), ImVec2(box.x + box.w - line_w, box.y), ImVec2(box.x + box.w, box.y + line_h));
        draw_corner(ImVec2(box.x, box.y + box.h), ImVec2(box.x + line_w, box.y + box.h), ImVec2(box.x, box.y + box.h - line_h));
        draw_corner(ImVec2(box.x + box.w, box.y + box.h), ImVec2(box.x + box.w - line_w, box.y + box.h), ImVec2(box.x + box.w, box.y + box.h - line_h));
    }

    bool GetEntityBox(C_BaseEntity* ent, Box& box_out) {
        ICollideable* collideable = ent->GetCollideable(); if (!collideable) return false;
        const Vector& origin = ent->GetRenderOrigin(); const Vector& min = collideable->OBBMins(); const Vector& max = collideable->OBBMaxs();
        Vector points[] = { origin + Vector(min.x, min.y, min.z), origin + Vector(min.x, max.y, min.z), origin + Vector(max.x, max.y, min.z), origin + Vector(max.x, min.y, min.z), origin + Vector(max.x, max.y, max.z), origin + Vector(min.x, max.y, max.z), origin + Vector(min.x, min.y, max.z), origin + Vector(max.x, min.y, max.z) };
        ImVec2 screen_points[8]; float left = 10000.f, top = 10000.f, right = -10000.f, bottom = -10000.f;
        for (int i = 0; i < 8; i++) { if (!WorldToScreen(points[i], screen_points[i])) return false; left = (std::min)(left, screen_points[i].x); top = (std::min)(top, screen_points[i].y); right = (std::max)(right, screen_points[i].x); bottom = (std::max)(bottom, screen_points[i].y); }
        if (left < 0 || top < 0 || right > g_ScreenWidth || bottom > g_ScreenHeight) return false;
        box_out.x = left; box_out.y = top; box_out.w = right - left; box_out.h = bottom - top;
        return true;
    }

    void DrawSkeleton(C_BasePlayer* ent, ImU32 color) {
        Matrix3x4 matrix[128]; if (!ent->SetupBones(matrix, 128, 0x00000100, interfaces::globalVars->curtime)) return;
        auto get_bone_screen = [&](int bone, ImVec2& out) -> bool { Vector pos(matrix[bone][0][3], matrix[bone][1][3], matrix[bone][2][3]); return WorldToScreen(pos, out); };
        int bones[][2] = { {6, 5}, {5, 4}, {4, 2}, {2, 0}, {5, 8}, {8, 9}, {9, 10}, {5, 13}, {13, 14}, {14, 15}, {0, 22}, {22, 23}, {23, 24}, {0, 25}, {25, 26}, {26, 27} };
        ImDrawList* drawList = ImGui::GetBackgroundDrawList(); for (auto& bone : bones) { ImVec2 p1, p2; if (get_bone_screen(bone[0], p1) && get_bone_screen(bone[1], p2)) drawList->AddLine(p1, p2, color, 1.0f); }
    }

    void Draw3DBox(C_BaseEntity* ent, ImU32 color) {
        ICollideable* collideable = ent->GetCollideable(); if (!collideable) return;
        const Vector& min = collideable->OBBMins(); const Vector& max = collideable->OBBMaxs(); Vector origin = ent->GetAbsOrigin();
        Vector points[8] = { Vector(min.x, min.y, min.z), Vector(min.x, max.y, min.z), Vector(max.x, max.y, min.z), Vector(max.x, min.y, min.z), Vector(min.x, min.y, max.z), Vector(min.x, max.y, max.z), Vector(max.x, max.y, max.z), Vector(max.x, min.y, max.z) };
        ImVec2 s[8]; for (int i = 0; i < 8; i++) { if (!WorldToScreen(origin + points[i], s[i])) return; }
        ImDrawList* drawList = ImGui::GetBackgroundDrawList(); for (int i = 0; i < 4; i++) { drawList->AddLine(s[i], s[(i + 1) % 4], color); drawList->AddLine(s[i + 4], s[((i + 1) % 4) + 4], color); drawList->AddLine(s[i], s[i + 4], color); }
    }

    void HandlePlayerESP(const CachedEntity& cached) {
        if (!cached.is_alive || (config::esp_dormant == false && cached.is_dormant)) return;
        float dist = (localPlayer->GetAbsOrigin() - cached.origin).Length() / 39.37f; if (dist > config::esp_max_dist_players) return;
        if (config::esp_players_visible_only && !IsVisible(cached.entity)) return;
        Box box; if (!GetEntityBox(cached.entity, box)) return;
        ImVec2 tl(box.x, box.y); ImVec2 br(box.x + box.w, box.y + box.h); ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(config::comp_box2d.color[0], config::comp_box2d.color[1], config::comp_box2d.color[2], config::comp_box2d.color[3]));
        if (cached.is_dormant) col = IM_COL32(150, 150, 150, 200);
        if (config::comp_box2d.enabled) {
            if (config::esp_box_type == 0) { drawList->AddRect(tl, br, col); drawList->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), IM_COL32(0, 0, 0, 255)); }
            else if (config::esp_box_type == 1) { drawList->AddRect(tl, br, col); drawList->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), IM_COL32(0, 0, 0, 255)); drawList->AddRect(ImVec2(tl.x + 1, tl.y + 1), ImVec2(br.x - 1, br.y - 1), IM_COL32(0, 0, 0, 255)); }
            else if (config::esp_box_type == 2) { DrawCornerBox(box, col); }
            else if (config::esp_box_type == 3) { Draw3DBox(cached.entity, col); }
        }
        if (config::comp_skeleton.enabled) DrawSkeleton(cached.entity, col);
        float font_size = calc_text_size(box); float top_off = 2.f, bot_off = 2.f, l_off = 4.f, r_off = 4.f; float l_text_y = 0.f, r_text_y = 0.f;
        auto draw_vertical_bar = [&](config::ESPComponent& comp, float fraction, ImU32 color) {
            if (!comp.enabled) return; float bar_w = 2.f; 
            if (comp.side == 0) { drawList->AddRectFilled(ImVec2(tl.x - 1, tl.y - top_off - 3.f), ImVec2(br.x + 1, tl.y - top_off + 1.f), IM_COL32(0, 0, 0, 180)); drawList->AddRectFilled(ImVec2(tl.x, tl.y - top_off - 2.f), ImVec2(tl.x + (box.w * fraction), tl.y - top_off), color); top_off += 4.f + comp.padding; }
            else if (comp.side == 1) { drawList->AddRectFilled(ImVec2(tl.x - 1, br.y + bot_off - 1), ImVec2(br.x + 1, br.y + bot_off + 3.f), IM_COL32(0, 0, 0, 180)); drawList->AddRectFilled(ImVec2(tl.x, br.y + bot_off), ImVec2(tl.x + (box.w * fraction), br.y + bot_off + 2.f), color); bot_off += 4.f + comp.padding; }
            else if (comp.side == 2) { drawList->AddRectFilled(ImVec2(tl.x - l_off - bar_w - 1, tl.y - 1), ImVec2(tl.x - l_off + 1, br.y + 1), IM_COL32(0, 0, 0, 180)); drawList->AddRectFilled(ImVec2(tl.x - l_off - bar_w, br.y - (box.h * fraction)), ImVec2(tl.x - l_off, br.y), color); l_off += bar_w + comp.padding + 2.f; }
            else if (comp.side == 3) { drawList->AddRectFilled(ImVec2(br.x + r_off - 1, tl.y - 1), ImVec2(br.x + r_off + bar_w + 1, br.y + 1), IM_COL32(0, 0, 0, 180)); drawList->AddRectFilled(ImVec2(br.x + r_off, br.y - (box.h * fraction)), ImVec2(br.x + r_off + bar_w, br.y), color); r_off += bar_w + comp.padding + 2.f; }
        };
        int hp = cached.health; int max_hp = 100; { std::shared_lock<std::shared_mutex> lock(entity_cache::data_mutex); if (entity_cache::player_max_health.count(cached.index)) max_hp = entity_cache::player_max_health[cached.index]; }
        if (max_hp <= 0) max_hp = 100; float hp_pct = std::clamp((float)hp / (float)max_hp, 0.f, 1.f); ImU32 hp_col = IM_COL32(255 * (1.0f - hp_pct), 255 * hp_pct, 0, 255); draw_vertical_bar(config::comp_health_bar, hp_pct, hp_col);
        if (cached.armor > 0) { float arm_pct = std::clamp((float)cached.armor / 100.f, 0.f, 1.f); ImU32 arm_col = ImGui::ColorConvertFloat4ToU32(ImVec4(config::comp_armor_bar.color[0], config::comp_armor_bar.color[1], config::comp_armor_bar.color[2], config::comp_armor_bar.color[3])); draw_vertical_bar(config::comp_armor_bar, arm_pct, arm_col); }
        auto draw_text = [&](config::ESPComponent& comp, const char* text, ImU32 override_color = 0) {
            if (!comp.enabled || !text || text[0] == '\0') return; ImU32 tcol = (override_color != 0) ? override_color : ImGui::ColorConvertFloat4ToU32(ImVec4(comp.color[0], comp.color[1], comp.color[2], comp.color[3]));
            if (comp.side == 0) { DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(tl.x + box.w / 2.0f, tl.y - top_off - font_size), tcol, text, true); top_off += font_size + comp.padding; }
            else if (comp.side == 1) { DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(tl.x + box.w / 2.0f, br.y + bot_off), tcol, text, true); bot_off += font_size + comp.padding; }
            else if (comp.side == 2) { ImVec2 t_size = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text); DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(tl.x - l_off - t_size.x, tl.y + l_text_y), tcol, text, false); l_text_y += font_size + comp.padding; }
            else if (comp.side == 3) { DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(br.x + r_off, tl.y + r_text_y), tcol, text, false); r_text_y += font_size + comp.padding; }
        };
        std::string name = cached.player_info.name; std::string rank = "", job = ""; ImColor team_col(255, 255, 255, 255);
        { std::shared_lock<std::shared_mutex> lock(entity_cache::data_mutex); if (entity_cache::player_names.count(cached.index)) name = entity_cache::player_names[cached.index]; if (entity_cache::player_ranks.count(cached.index)) rank = entity_cache::player_ranks[cached.index]; if (entity_cache::player_jobs.count(cached.index)) job = entity_cache::player_jobs[cached.index]; if (entity_cache::player_team_colors.count(cached.index)) team_col = entity_cache::player_team_colors[cached.index]; }
        if (config::friends_list[cached.index]) { name = "[F] " + name; }
        draw_text(config::comp_name, name.c_str()); draw_text(config::comp_user_group, rank.c_str()); draw_text(config::comp_job, job.c_str(), team_col);
        if (hp > 0) { std::string hp_str = std::to_string(hp) + "%"; draw_text(config::comp_health_pct, hp_str.c_str(), hp_col); }
        if (config::comp_distance.enabled) { int d_val = (int)dist; draw_text(config::comp_distance, (std::to_string(d_val) + "m").c_str()); }
        if (config::comp_weapon.enabled) { draw_text(config::comp_weapon, cached.network_name); }
    }

    void HandleEntityESP(const CachedEntity& cached, config::ESPComponent& comp, bool is_generic_ent = false) {
        if (cached.is_dormant) return;
        float dist_len = (localPlayer->GetAbsOrigin() - cached.origin).Length() / 39.37f; if (dist_len > config::esp_max_dist_entities) return;
        bool is_custom = false; config::CustomEntityESP custom_cfg; std::string d_name = cached.network_name;
        { std::shared_lock<std::shared_mutex> lock(entity_cache::data_mutex); if (entity_cache::entity_classes.count(cached.index)) d_name = entity_cache::entity_classes[cached.index]; }
        if (config::custom_entity_esp.count(d_name) && config::custom_entity_esp[d_name].enabled) { custom_cfg = config::custom_entity_esp[d_name]; is_custom = true; }
        if (is_generic_ent && !is_custom) return; if (!is_generic_ent && !is_custom && !comp.enabled) return;
        if (config::esp_entities_visible_only && !IsVisible(cached.entity)) return;
        Box box; if (!GetEntityBox(cached.entity, box)) return;
        ImVec2 tl(box.x, box.y); ImVec2 br(box.x + box.w, box.y + box.h); ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        float font_size = calc_text_size(box); float top_off = 2.f, bot_off = 2.f, l_off = 4.f, r_off = 4.f; float l_text_y = 0.f, r_text_y = 0.f;
        auto draw_box = [&](ImU32 color, bool is_3d = false) { if (is_3d) Draw3DBox(cached.entity, color); else { drawList->AddRect(tl, br, color); drawList->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), IM_COL32(0, 0, 0, 255)); } };
        auto draw_text = [&](const char* text, int side, float padding, ImU32 color) {
            if (!text || text[0] == '\0') return;
            if (side == 0) { DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(tl.x + box.w / 2.0f, tl.y - top_off - font_size), color, text, true); top_off += font_size + padding; }
            else if (side == 1) { DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(tl.x + box.w / 2.0f, br.y + bot_off), color, text, true); bot_off += font_size + padding; }
            else if (side == 2) { ImVec2 t_size = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text); DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(tl.x - l_off - t_size.x, tl.y + l_text_y), color, text, false); l_text_y += font_size + padding; }
            else if (side == 3) { DrawString(drawList, ImGui::GetFont(), font_size, ImVec2(br.x + r_off, tl.y + r_text_y), color, text, false); r_text_y += font_size + padding; }
        };
        if (is_custom) {
            if (custom_cfg.box2d.enabled) draw_box(ImGui::ColorConvertFloat4ToU32(ImVec4(custom_cfg.box2d.color[0], custom_cfg.box2d.color[1], custom_cfg.box2d.color[2], custom_cfg.box2d.color[3])));
            if (custom_cfg.box3d.enabled) draw_box(ImGui::ColorConvertFloat4ToU32(ImVec4(custom_cfg.box3d.color[0], custom_cfg.box3d.color[1], custom_cfg.box3d.color[2], custom_cfg.box3d.color[3])), true);
            if (custom_cfg.name.enabled) draw_text(d_name.c_str(), custom_cfg.name.side, custom_cfg.name.padding, ImGui::ColorConvertFloat4ToU32(ImVec4(custom_cfg.name.color[0], custom_cfg.name.color[1], custom_cfg.name.color[2], custom_cfg.name.color[3])));
            if (custom_cfg.distance.enabled) { int dist = (int)dist_len; draw_text((std::to_string(dist) + "m").c_str(), custom_cfg.distance.side, custom_cfg.distance.padding, ImGui::ColorConvertFloat4ToU32(ImVec4(custom_cfg.distance.color[0], custom_cfg.distance.color[1], custom_cfg.distance.color[2], custom_cfg.distance.color[3]))); }
        } else {
            ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(comp.color[0], comp.color[1], comp.color[2], comp.color[3]));
            if (config::comp_box2d.enabled) { if (config::esp_box_type == 0 || config::esp_box_type == 1) draw_box(col); else if (config::esp_box_type == 2) DrawCornerBox(box, col); else if (config::esp_box_type == 3) Draw3DBox(cached.entity, col); }
            draw_text(cached.network_name, 0, 2.f, col);
        }
    }

    void run() {
        if (!interfaces::engine->IsInGame() || !localPlayer) return;
        g_ViewMatrix = interfaces::engine->WorldToScreenMatrix(); interfaces::engine->GetScreenSize(g_ScreenWidth, g_ScreenHeight);
       
        static auto last_spam_time = std::chrono::steady_clock::now();
        if (config::chat_spam_enabled && !config::chat_spam_list.empty()) {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> diff = now - last_spam_time;
            if (diff.count() > config::chat_spam_delay) {
                static int spam_idx = 0;
                if (spam_idx >= (int)config::chat_spam_list.size()) spam_idx = 0;
                
                std::string msg = config::chat_spam_list[spam_idx];
                std::string cmd = config::chat_spam_ooc ? "say /ooc " + msg : "say " + msg;
                
                interfaces::engine->ClientCmd_Unrestricted(cmd.c_str());
                
                spam_idx++;
                last_spam_time = now;
            }
        }

        if (!config::esp_enabled) return; std::shared_lock<std::shared_mutex> lock(entity_cache::cache_mutex);
        if (config::esp_cat_players) for (const auto& p : entity_cache::players) HandlePlayerESP(p);
        if (config::esp_cat_npcs) for (const auto& n : entity_cache::npcs) HandleEntityESP(n, config::comp_name);
        if (config::esp_cat_weapons) for (const auto& w : entity_cache::weapons) HandleEntityESP(w, config::comp_weapon);
        for (const auto& e : entity_cache::entities) HandleEntityESP(e, config::comp_name, true);
    }
}
