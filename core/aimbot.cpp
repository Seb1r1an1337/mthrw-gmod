#include "aimbot.hpp"
#include "entity_cache.hpp"
#include "config.hpp"
#include "../interfaces/cenginetrace.hpp"

namespace aimbot {

    Vector GetBonePos(C_BasePlayer* ent, int bone) {
        Matrix3x4 matrix[128];
        if (ent->SetupBones(matrix, 128, 0x00000100, interfaces::globalVars->curtime)) {
            return Vector(matrix[bone][0][3], matrix[bone][1][3], matrix[bone][2][3]);
        }
        return ent->GetAbsOrigin();
    }

    bool IsVisible(const Vector& start, const Vector& end, C_BasePlayer* target) noexcept {
        CTrace trace;
        TraceFilterSimple filter(localPlayer);
        Ray_t ray;
        ray.Init(start, end);

        interfaces::trace->TraceRay(ray, MASK_SHOT, &filter, &trace);

        return trace.entity == target || trace.fraction >= 0.97f;
    }

    void run(CUserCmd* cmd) noexcept {
        if (!localPlayer || !localPlayer->IsAlive())
            return;

        // Triggerbot
        if (config::triggerbot) {
            Vector start = localPlayer->GetAbsOrigin() + localPlayer->GetViewOffset();
            Vector end = start + cmd->viewangles.Forward() * 8192.f;

            CTrace trace;
            TraceFilterSimple filter(localPlayer);
            Ray_t ray;
            ray.Init(start, end);

            interfaces::trace->TraceRay(ray, MASK_SHOT, &filter, &trace);

            if (trace.entity) {
                bool should_shoot = false;
                
                // Safe check against cached entities
                std::shared_lock<std::shared_mutex> lock(entity_cache::cache_mutex);
                
                if (config::aim_players) {
                    for (const auto& p : entity_cache::players) {
                        if (p.entity == trace.entity && p.is_alive && !p.is_dormant) {
                            if (!(config::ignore_friends && config::friends_list[p.index])) {
                                should_shoot = true;
                            }
                            break;
                        }
                    }
                }
                
                if (!should_shoot && config::aim_npcs) {
                    for (const auto& n : entity_cache::npcs) {
                        if (n.entity == trace.entity && !n.is_dormant) {
                            should_shoot = true;
                            break;
                        }
                    }
                }

                if (should_shoot) {
                    static int delay_ticks = 0;
                    if (delay_ticks >= config::trigger_delay / 15) { // Approx ticks
                        cmd->buttons |= CUserCmd::IN_ATTACK;
                        delay_ticks = 0;
                    } else {
                        delay_ticks++;
                    }
                }
            }
        }

        if (!config::aim_enabled) return;

        // Key check
        if (!(GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_XBUTTON1))) return;

        Vector eyePos = localPlayer->GetAbsOrigin() + localPlayer->GetViewOffset();
        Angle viewAngles;
        interfaces::engine->GetViewAngles(viewAngles);

        C_BasePlayer* bestTarget = nullptr;
        float bestFov = config::fov;
        Vector bestHitboxPos;

        std::shared_lock<std::shared_mutex> lock(entity_cache::cache_mutex);

        // Target Players
        if (config::aim_players) {
            for (const auto& player : entity_cache::players) {
                if (!player.is_alive || player.is_dormant) continue;
                if (config::ignore_friends && config::friends_list[player.index]) continue;
                // if (config::ignore_team && player.entity->GetTeam() == localPlayer->GetTeam()) continue;

                int bone = bone_head; // Default
                if (config::hitbox_priority == 1) bone = bone_spine_1; // Chest
                else if (config::hitbox_priority == 2) bone = bone_pelvis;

                Vector hitboxPos = GetBonePos(player.entity, bone);
                
                if (config::esp_vis_check && !IsVisible(eyePos, hitboxPos, player.entity))
                    continue;

                Angle aimAngle = Math::CalculateAngle(eyePos, hitboxPos);
                float fov = Math::GetFov(viewAngles, aimAngle);

                if (fov < bestFov) {
                    bestFov = fov;
                    bestTarget = player.entity;
                    bestHitboxPos = hitboxPos;
                }
            }
        }

        if (config::aim_npcs) {
            for (const auto& npc : entity_cache::npcs) {
                if (npc.is_dormant) continue;

                Vector hitboxPos = npc.origin + Vector(0, 0, 40);
                
                if (config::esp_vis_check && !IsVisible(eyePos, hitboxPos, npc.entity))
                    continue;

                Angle aimAngle = Math::CalculateAngle(eyePos, hitboxPos);
                float fov = Math::GetFov(viewAngles, aimAngle);

                if (fov < bestFov) {
                    bestFov = fov;
                    bestTarget = npc.entity;
                    bestHitboxPos = hitboxPos;
                }
            }
        }

        if (bestTarget) {
            Angle aimAngle = Math::CalculateAngle(eyePos, bestHitboxPos);
            
            if (config::no_recoil_bool) {
                aimAngle -= localPlayer->GetAimPunch() * 2.0f;
            }

            aimAngle.FixAngles();

            if (config::smoothing > 0.1f) {
                Angle delta = aimAngle - viewAngles;
                delta.FixAngles();
                aimAngle = viewAngles + delta / (config::smoothing / 2.0f);
                aimAngle.FixAngles();
            }

            if (config::aim_mode == 2) { // Silent
                cmd->viewangles = aimAngle;
            } else if (config::aim_mode == 1) { // Viewangles
                interfaces::engine->SetViewAngles(aimAngle);
            }

            if (config::auto_fire) {
                cmd->buttons |= CUserCmd::IN_ATTACK;
            }
        }
    }
}
