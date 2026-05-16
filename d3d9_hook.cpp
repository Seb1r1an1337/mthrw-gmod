#include "d3d9_hook.hpp"
#include "core/fonts.hpp"
#include "core/menufont.hpp"
#include "core/visuals.hpp"
#include "core/theme.hpp"
#include "core/entity_cache.hpp"
#include "core/lua_api.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool init = false;
TextEditor editor;

static WNDPROC ogWndProc = NULL;
static HWND window = NULL;

using _Present = long(__stdcall*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
_Present ogPresent = nullptr;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
        d3d9hook::menuOpened = !d3d9hook::menuOpened;
    }
    if (d3d9hook::menuOpened) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }
    return CallWindowProcA(ogWndProc, hWnd, uMsg, wParam, lParam);
}

void DrawEnvironmentWindow() {
    if (!config::wnd_environment_list) return;

    static char player_search[64] = "";
    static char entity_search[64] = "";

    ImGui::SetNextWindowSize({ 500, 400 }, ImGuiCond_Once);
    if (ImGui::Begin("Environment List", &config::wnd_environment_list, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("EnvTabs")) {
            if (ImGui::BeginTabItem("Players")) {
                ImGui::InputTextWithHint("##player_search", "Search player...", player_search, sizeof(player_search));
                ImGui::Separator();

                std::shared_lock<std::shared_mutex> lock(entity_cache::cache_mutex);
                if (ImGui::BeginTable("##players_table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Rank");
                    ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 40.f);
                    ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthFixed, 40.f);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 60.f);
                    ImGui::TableHeadersRow();

                    C_BasePlayer* local_ent = (C_BasePlayer*)interfaces::entityList->GetClientEntity(interfaces::engine->GetLocalPlayer());
                    Vector local_pos = local_ent ? local_ent->GetAbsOrigin() : Vector(0, 0, 0);

                    for (const auto& player : entity_cache::players) {
                        if (player_search[0] != '\0' && !strstr(player.player_info.name, player_search)) continue;

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::PushID(player.index);

                        ImVec4 color = ImVec4(1, 1, 1, 1);
                        if (config::friends_list[player.index]) color = ImVec4(0, 1, 0, 1);
                        else if (config::priority_list[player.index]) color = ImVec4(1, 0, 0, 1);

                        ImGui::TextColored(color, "%s", player.player_info.name);

                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                            ImGui::OpenPopup("player_ctx_menu");
                        }

                        if (ImGui::BeginPopup("player_ctx_menu")) {
                            ImGui::Text("Options for %s", player.player_info.name);
                            ImGui::Separator();
                            if (ImGui::Checkbox("Friend", &config::friends_list[player.index])) {
                                if (config::friends_list[player.index]) config::priority_list[player.index] = false;
                            }
                            if (ImGui::Checkbox("Priority", &config::priority_list[player.index])) {
                                if (config::priority_list[player.index]) config::friends_list[player.index] = false;
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::TableNextColumn();
                        std::string rank = "User";
                        {
                            std::shared_lock<std::shared_mutex> data_lock(entity_cache::data_mutex);
                            if (entity_cache::player_ranks.count(player.index)) rank = entity_cache::player_ranks[player.index];
                        }
                        ImGui::Text("%s", rank.c_str());

                        ImGui::TableNextColumn(); ImGui::Text("%d", player.health);
                        ImGui::TableNextColumn();
                        int distance = local_ent ? (int)(local_pos - player.origin).Length() / 39.37f : 0;
                        ImGui::Text("%dm", distance);
                        ImGui::TableNextColumn(); ImGui::Text("%s", player.is_alive ? (player.is_dormant ? "Dormant" : "Alive") : "Dead");
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Entities")) {
                ImGui::InputTextWithHint("##entity_search", "Search class...", entity_search, sizeof(entity_search));
                ImGui::Separator();

                std::shared_lock<std::shared_mutex> lock(entity_cache::cache_mutex);
                if (ImGui::BeginTable("##entities_table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Class Name");
                    ImGui::TableHeadersRow();

                    std::set<std::string> unique_classes;
                    auto collect_unique = [&](const auto& list) {
                        for (const auto& ent : list) {
                            std::string name = ent.network_name ? ent.network_name : "Unknown";
                            {
                                std::shared_lock<std::shared_mutex> data_lock(entity_cache::data_mutex);
                                if (entity_cache::entity_classes.count(ent.index)) name = entity_cache::entity_classes[ent.index];
                            }
                            if (!name.empty() && name != "Unknown" && name != "class C_BaseEntity") unique_classes.insert(name);
                        }
                    };

                    collect_unique(entity_cache::entities);
                    collect_unique(entity_cache::npcs);
                    collect_unique(entity_cache::weapons);

                    for (const auto& cls_name : unique_classes) {
                        if (entity_search[0] != '\0' && !strstr(cls_name.c_str(), entity_search)) continue;

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::PushID(cls_name.c_str());

                        ImVec4 color = (config::custom_entity_esp.count(cls_name) && config::custom_entity_esp[cls_name].enabled) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 1, 1, 1);
                        ImGui::TextColored(color, "%s", cls_name.c_str());

                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) ImGui::OpenPopup("ent_ctx_menu");

                        if (ImGui::BeginPopup("ent_ctx_menu")) {
                            ImGui::Text("Settings for ALL %s", cls_name.c_str());
                            ImGui::Separator();
                            auto& esp = config::custom_entity_esp[cls_name];
                            ImGui::Checkbox("Enable Class ESP", &esp.enabled);

                            if (esp.enabled) {
                                auto draw_sub_cfg = [](const char* label, config::CustomESPComponent& comp, bool detailed = true) {
                                    ImGui::PushID(label);
                                    ImGui::Checkbox(label, &comp.enabled);
                                    if (ImGui::IsItemHovered()) {
                                        ImGui::SetTooltip("Right-click for settings");
                                        if (ImGui::IsMouseClicked(1)) ImGui::OpenPopup("sub_cfg_pop");
                                    }
                                    if (ImGui::BeginPopup("sub_cfg_pop")) {
                                        ImGui::Text("%s Settings", label); ImGui::Separator();
                                        if (detailed) {
                                            const char* sides[] = { "Top", "Bottom", "Left", "Right" };
                                            ImGui::Combo("Side", &comp.side, sides, IM_ARRAYSIZE(sides));
                                            ImGui::SliderFloat("Padding", &comp.padding, 0.f, 20.f);
                                        }
                                        ImGui::ColorEdit4("Color", comp.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                                        ImGui::EndPopup();
                                    }
                                    ImGui::PopID();
                                };

                                draw_sub_cfg("2D Box", esp.box2d, false);
                                draw_sub_cfg("3D Box", esp.box3d, false);
                                draw_sub_cfg("Name", esp.name, true);
                                draw_sub_cfg("Distance", esp.distance, true);
                            }
                            ImGui::EndPopup();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void DrawChatSpamWindow() {
    if (!config::wnd_chat_spam) return;

    static char msg_buf[256] = "";
    static int selected_idx = -1;

    ImGui::SetNextWindowSize({ 450, 500 }, ImGuiCond_Once);
    if (ImGui::Begin("Chat Spam Editor", &config::wnd_chat_spam, ImGuiWindowFlags_NoCollapse)) {
        ImGui::InputTextWithHint("##msg_input", "Message", msg_buf, sizeof(msg_buf));
        
        if (ImGui::Button("Add To List")) {
            if (msg_buf[0] != '\0') {
                config::chat_spam_list.push_back(msg_buf);
                msg_buf[0] = '\0';
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove From List")) {
            if (selected_idx >= 0 && selected_idx < (int)config::chat_spam_list.size()) {
                config::chat_spam_list.erase(config::chat_spam_list.begin() + selected_idx);
                selected_idx = -1;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            config::chat_spam_list.clear();
            selected_idx = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) { }
        ImGui::SameLine();
        if (ImGui::Button("Save To File")) { }

        ImGui::Checkbox("Save After Modification", &config::chat_spam_save_on_mod);
        ImGui::Checkbox("OOC Mode", &config::chat_spam_ooc);

        if (ImGui::BeginTabBar("SpamTabs")) {
            if (ImGui::BeginTabItem("Chat Spam")) {
                ImGui::BeginChild("##spam_list_child", ImVec2(0, 0), true);
                for (int i = 0; i < (int)config::chat_spam_list.size(); i++) {
                    if (ImGui::Selectable(config::chat_spam_list[i].c_str(), selected_idx == i)) {
                        selected_idx = i;
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

long __stdcall detouredPresent(IDirect3DDevice9* pDevice, const RECT* rect1, const RECT* rect2, HWND hwnd, const RGNDATA* rgndata) {
    if (!init) {
        d3d9hook::renderer = pDevice;
        ImGui::CreateContext();
        theme::apply();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigWindowsResizeFromEdges = false;
        d3d9hook::defaultFont = io.Fonts->AddFontFromMemoryTTF(dataRoboto, sizeof(dataRoboto), 16.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        d3d9hook::editorFont = io.Fonts->AddFontFromMemoryTTF(dataRobotoMono, sizeof(dataRobotoMono), 14.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        
        if (sizeof(dataRaleway) > 10) {
            d3d9hook::menuFont = io.Fonts->AddFontFromMemoryTTF(dataRaleway, sizeof(dataRaleway), 16.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        } else {
            d3d9hook::menuFont = d3d9hook::defaultFont;
        }

        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX9_Init(pDevice);

        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        init = true;
    }

    if (config::anti_screenshot && interfaces::engine->IsTakingScreenshot()) {
        return ogPresent(pDevice, rect1, rect2, hwnd, rgndata);
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (d3d9hook::menuOpened) {
        ImGui::PushFont(d3d9hook::menuFont);
        if (ImGui::BeginMainMenuBar()) {
            ImGui::Text("methamphetamine.solutions v0.0.5 PTB");
            ImGui::Separator();
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Environment List", nullptr, &config::wnd_environment_list);
                ImGui::MenuItem("Lua Executor", nullptr, &config::wnd_lua_executor);
                ImGui::MenuItem("Chat Spam", nullptr, &config::wnd_chat_spam);
                ImGui::MenuItem("Spectator List", nullptr, &config::wnd_spectators);
                ImGui::MenuItem("Debug Console", nullptr, &config::wnd_console);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        DrawEnvironmentWindow();
        DrawChatSpamWindow();

        if (config::wnd_lua_executor) {
            ImGui::SetNextWindowSize({ 600, 450 }, ImGuiCond_Once);
            if (ImGui::Begin("Lua Executor", &config::wnd_lua_executor)) {
                ImGui::Text("Advanced Lua Executor (luaL_loadbufferx)");
                ImGui::InputText("Spoof Chunkname", config::lua_chunkname, 32);
                if (ImGui::Button("Execute Buffer", ImVec2(120, 25))) lualoader::Execute(editor.GetText());
                ImGui::SameLine();
                if (ImGui::Button("Clear", ImVec2(80, 25))) editor.SetText("");
                ImGui::Spacing();
                editor.Render("LuaEditor", ImVec2(0, 0), true);
            }
            ImGui::End();
        }
        
        ImGui::SetNextWindowSize({ 700, 520 }, ImGuiCond_Always);
        if (ImGui::Begin("methamphetamine.solutions (x64)", &d3d9hook::menuOpened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
            if (ImGui::BeginTabBar("MainTabs")) {
                if (ImGui::BeginTabItem("ESP")) {
                    ImGui::Spacing();
                    ImGui::Checkbox("Master Switch", &config::esp_enabled);
                    ImGui::Separator();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 345.f);

                    float group_height = ImGui::GetContentRegionAvail().y - 25.f;

                    ImGui::Text("Entities");
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.07f, 1.00f));
                    if (ImGui::BeginChild("##entities_group", ImVec2(335, group_height), true, ImGuiWindowFlags_NoScrollbar)) {
                        if (ImGui::BeginTabBar("SubEntities")) {
                            auto draw_oink_row = [](const char* label, config::ESPComponent& comp, bool is_box = false, bool has_combo = true) {
                                ImGui::PushID(label);
                                ImGui::Checkbox("##cb", &comp.enabled); ImGui::SameLine();
                                ImGui::Text(label);
                                float pane_w = ImGui::GetWindowContentRegionMax().x;
                                ImGui::SameLine(pane_w - 25.f);
                                ImGui::ColorEdit4("##col", comp.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                                if (has_combo) {
                                    ImGui::SameLine(pane_w - 135.f);
                                    ImGui::SetNextItemWidth(105.f);
                                    if (is_box) {
                                        const char* types[] = { "Flat", "Bounding", "Corners", "3D" };
                                        ImGui::Combo("##type", &config::esp_box_type, types, IM_ARRAYSIZE(types));
                                    } else {
                                        const char* sides[] = { "Top", "Bottom", "Left", "Right" };
                                        ImGui::Combo("##side", &comp.side, sides, IM_ARRAYSIZE(sides));
                                    }
                                }
                                ImGui::PopID();
                            };

                            if (ImGui::BeginTabItem("Players")) {
                                ImGui::Spacing();
                                draw_oink_row("Box", config::comp_box2d, true);
                                draw_oink_row("Name", config::comp_name);
                                draw_oink_row("Health [Txt]", config::comp_health_pct);
                                draw_oink_row("Health [Bar]", config::comp_health_bar);
                                draw_oink_row("Armor [Bar]", config::comp_armor_bar);
                                draw_oink_row("Weapon", config::comp_weapon);
                                draw_oink_row("User Group", config::comp_user_group);
                                draw_oink_row("Job (DarkRP)", config::comp_job);
                                draw_oink_row("Distance", config::comp_distance);
                                draw_oink_row("Skeleton", config::comp_skeleton, false, false);
                                ImGui::Separator();
                                ImGui::Text("Filtering");
                                ImGui::Checkbox("Visible Only", &config::esp_players_visible_only);
                                ImGui::PushItemWidth(ImGui::GetWindowWidth() - 20.f);
                                ImGui::SliderFloat("##dist", &config::esp_max_dist_players, 0.f, 5000.f, "Max Distance: %.0fm");
                                ImGui::PopItemWidth();
                                ImGui::EndTabItem();
                            }
                            if (ImGui::BeginTabItem("NPCs")) {
                                ImGui::Spacing();
                                ImGui::Checkbox("Enabled NPCs", &config::esp_cat_npcs);
                                draw_oink_row("NPC Name", config::comp_name);
                                ImGui::EndTabItem();
                            }
                            if (ImGui::BeginTabItem("Entity")) {
                                ImGui::Spacing();
                                ImGui::Checkbox("Dormant ESP", &config::esp_dormant);
                                ImGui::TextWrapped("Select specific entities in Environment List.");
                                ImGui::EndTabItem();
                            }
                            ImGui::EndTabBar();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::NextColumn();

                    ImGui::Text("Other");
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.07f, 1.00f));
                    if (ImGui::BeginChild("##other_group", ImVec2(335, group_height), true, ImGuiWindowFlags_NoScrollbar)) {
                        ImGui::Spacing();
                        ImGui::Checkbox("No Visual Recoil", &config::no_recoil_bool);
                        ImGui::Checkbox("No Particle Effects & Gore", &config::no_spread_bool);
                        ImGui::Checkbox("No Rendering Limitations", &config::esp_dormant);
                        ImGui::Checkbox("Draw FoV", &config::draw_fov);
                        float pw = ImGui::GetWindowContentRegionMax().x;
                        ImGui::SameLine(pw - 25.f);
                        ImGui::ColorEdit4("##fovcol", config::fov_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);

                        auto draw_chams_row = [](const char* label, bool* enabled, int* mat) {
                            ImGui::PushID(label);
                            ImGui::Checkbox(label, enabled);
                            float pane_w = ImGui::GetWindowContentRegionMax().x;
                            ImGui::SameLine(pane_w - 135.f);
                            ImGui::SetNextItemWidth(105.f);
                            ImGui::Combo("##mat", mat, "Normal\0Flat\0Glow\0Wire\0");
                            ImGui::SameLine(pane_w - 25.f);
                            ImGui::ColorEdit4("##col", config::comp_box2d.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                            ImGui::PopID();
                        };
                        draw_chams_row("Hands Chams", &config::hand_chams, &config::chams_mat);
                        draw_chams_row("Weapon Chams", &config::silent_aim, &config::chams_mat);
                        ImGui::Separator();
                        ImGui::Text("Radar Style");
                        ImGui::SetNextItemWidth(200.f);
                        ImGui::Combo("##radar", &config::radar_style, "Disabled\0Circular\0Square\0");
                        ImGui::Checkbox("Shorten Entity Names", &config::esp_dormant);
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::Columns(1);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Aimbot")) {
                    ImGui::Spacing();
                    ImGui::Checkbox("Enabled", &config::aim_enabled);
                    ImGui::Separator();
                    ImGui::Columns(2, nullptr, false);
                    ImGui::Text("Main Settings");
                    ImGui::Combo("Target Mode", &config::aim_mode, "Off\0Client\0Server\0");
                    ImGui::SliderFloat("Fov", &config::fov, 0.f, 180.f, "%.0f");
                    ImGui::SliderFloat("Smooth", &config::smoothing, 0.f, 100.f, "%.0f");
                    ImGui::Checkbox("Auto Fire", &config::auto_fire);
                    ImGui::NextColumn();
                    ImGui::Text("Extra");
                    ImGui::Checkbox("Auto Wall", &config::auto_wall);
                    ImGui::Checkbox("Silent Aim", &config::silent_aim);
                    ImGui::Columns(1);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Misc")) {
                    ImGui::Spacing();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 340.f);

                    float group_height = ImGui::GetContentRegionAvail().y - 25.f;

                    ImGui::Text("Movement");
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.07f, 1.00f));
                    if (ImGui::BeginChild("##movement_group", ImVec2(325, group_height), true, ImGuiWindowFlags_NoScrollbar)) {
                        ImGui::Spacing();
                        ImGui::Checkbox("Bunny Hop", &config::bunnyhop);
                        ImGui::Text("Bunny Hop Mode");
                        ImGui::SetNextItemWidth(-1);
                        ImGui::Combo("##bhop_mode", &config::bhop_mode, "Standard\0Safe\0");

                        ImGui::Text("Max Hops [Safe]");
                        ImGui::SetNextItemWidth(-1);
                        ImGui::SliderInt("##max_hops", &config::max_hops, 0, 20);

                        ImGui::Checkbox("Auto Strafe", &config::autostrafe);
                        ImGui::Text("Auto Strafe Mode");
                        ImGui::SetNextItemWidth(-1);
                        ImGui::Combo("##strafe_mode", &config::autostrafe_mode, "Legit\0Rage\0");
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::NextColumn();

                    ImGui::Text("Other");
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.07f, 1.00f));
                    if (ImGui::BeginChild("##comm_group", ImVec2(325, group_height), true, ImGuiWindowFlags_NoScrollbar)) {
                        ImGui::Spacing();
                        ImGui::Checkbox("Chat Spam", &config::chat_spam_enabled);
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Right-click for settings");
                            if (ImGui::IsMouseClicked(1)) ImGui::OpenPopup("spam_settings_pop");
                        }
                        if (ImGui::BeginPopup("spam_settings_pop")) {
                            ImGui::Text("Spam Settings"); ImGui::Separator();
                            ImGui::SliderFloat("Delay", &config::chat_spam_delay, 0.5f, 10.0f, "%.1fs");
                            ImGui::EndPopup();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::Columns(1);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Config")) {
                    ImGui::Text("Config System Placeholder");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        ImGui::PopFont();
    }

    if (interfaces::engine->IsInGame() && interfaces::engine->IsConnected() && !interfaces::engine->IsDrawingLoadingImage()) {
        entity_cache::update();
        visuals::run();
        if (config::draw_fov && config::aim_enabled) {
            int sw, sh; interfaces::engine->GetScreenSize(sw, sh);
            float radius = (config::fov / 90.0f) * (sw / 2.0f);
            ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(sw / 2.0f, sh / 2.0f), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(config::fov_color[0], config::fov_color[1], config::fov_color[2], config::fov_color[3])), 64);
        }
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    return ogPresent(pDevice, rect1, rect2, hwnd, rgndata);
}

void d3d9hook::Init() noexcept {
    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success) {
        window = FindWindowA("Valve001", 0);
        ogWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)WndProc));
        kiero::bind(17, reinterpret_cast<void**>(&ogPresent), &detouredPresent);
    }
}

void d3d9hook::Shutdown() noexcept {
    if (window && ogWndProc)
        SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)ogWndProc);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    kiero::unbind(17);
    kiero::shutdown();
}