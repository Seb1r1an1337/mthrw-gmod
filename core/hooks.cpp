#include "hooks.hpp"
#include "visuals.hpp"
#include "../interfaces/cmovehelper.hpp"
#include "../interfaces/cbasecombatweapon.hpp"
#include "../interfaces/gameevents.hpp"
#include "aimbot.hpp"
#include "entity_cache.hpp"

using _CreateMove = bool(__thiscall*)(IClientModeNormal*, float, CUserCmd*);
_CreateMove ogCreateMove = nullptr;

using _LuaHook = lua_State * (__cdecl*)(void*, void*);
_LuaHook ogLuaHook = nullptr;

using _ReadPixels = void(__thiscall*)(IMatRenderContext*, int, int, int, int, unsigned char*, ImageFormat);
_ReadPixels ogReadPixels = nullptr;

using _ReadPixelsAndStretch = void(__thiscall*)(IMatRenderContext*, void*, void*, unsigned char*, ImageFormat, int);
_ReadPixelsAndStretch ogReadPixelsAndStretch = nullptr;

using _DrawModelExecute = void(__thiscall*)(CModelRender*, const DrawModelState_t&, const ModelRenderInfo_t&, Matrix3x4*);
_DrawModelExecute ogDrawModelExecute = nullptr;

static float cl_sidespeed = 10000;

CMoveData moveData;

bool __stdcall detourCreateMove(void*, float flInputSampleTime, CUserCmd* cmd) {
	localPlayer = (C_BasePlayer*)interfaces::entityList->GetClientEntity(interfaces::engine->GetLocalPlayer());

    entity_cache::update_lua_data();

	const auto result = ogCreateMove(interfaces::clientMode, flInputSampleTime, cmd);
	if (!cmd || !cmd->command_number)
		return result;

	if (localPlayer) {
		float oldCurtime = interfaces::globalVars->curtime;
		float oldFrametime = interfaces::globalVars->frametime;

		interfaces::globalVars->curtime = localPlayer->GetTickBase() * interfaces::globalVars->interval_per_tick;
		interfaces::globalVars->frametime = interfaces::globalVars->interval_per_tick;

		config::currentvelocity = localPlayer->GetVelocity().Length();
		
        if (config::bunnyhop && (cmd->buttons & (1 << 1))) {
            static bool jumped_last_tick = false;
            static bool should_fake_jump = false;

            if (!jumped_last_tick && should_fake_jump) {
                should_fake_jump = false;
                cmd->buttons |= (1 << 1);
            } else if (localPlayer->GetFlags() & (1 << 0)) {
                jumped_last_tick = true;
                should_fake_jump = true;
            } else {
                cmd->buttons &= ~(1 << 1);
                jumped_last_tick = false;
            }
        }

        if (config::autostrafe && !(localPlayer->GetFlags() & (1 << 0)) && localPlayer->GetMoveType() != 9) {
            Vector velocity = localPlayer->GetVelocity();
            float speed2d = velocity.Length2D();

            if (speed2d > 5.0f) {
                Angle view_angles;
                interfaces::engine->GetViewAngles(view_angles);

                static bool side_switch = false;
                side_switch = !side_switch;

                cmd->forwardmove = 0.f;
                cmd->sidemove = side_switch ? 450.f : -450.f;

                float velocity_yaw = std::atan2(velocity.y, velocity.x) * (180.0f / 3.14159265f);
                float rotation = std::clamp<float>(57.2957795f * std::atan2(15.f, speed2d), 0.f, 90.f);
                
                float yaw_delta = view_angles.y - velocity_yaw;
                while (yaw_delta > 180.f) yaw_delta -= 360.f;
                while (yaw_delta < -180.f) yaw_delta += 360.f;

                float ideal_yaw = (side_switch ? rotation : -rotation) + (std::abs(yaw_delta) < 5.f ? velocity_yaw : view_angles.y);
                float rad_diff = (view_angles.y - ideal_yaw) * (3.14159265f / 180.0f);

                float cos_d = std::cos(rad_diff);
                float sin_d = std::sin(rad_diff);

                float old_f = cmd->forwardmove;
                float old_s = cmd->sidemove;

                cmd->forwardmove = (cos_d * old_f) - (sin_d * old_s);
                cmd->sidemove = (sin_d * old_f) + (cos_d * old_s);
            }
        }

        aimbot::run(cmd);

        // Chat Queue Processing
        static float next_msg_time = 0.f;
        float curtime = interfaces::globalVars->curtime;
        if (curtime > next_msg_time) {
            std::lock_guard<std::mutex> lock(visuals::chat_mutex);
            if (!visuals::chat_queue.empty()) {
                std::string chat_msg = visuals::chat_queue.front();
                visuals::chat_queue.pop();
                
                std::string full_cmd = "say \"" + chat_msg + "\"";
                interfaces::engine->ClientCmd_Unrestricted(full_cmd.c_str());
                
                next_msg_time = curtime + 0.1f;
            }
        }
	}

	return false;
}

lua_State* __cdecl detourLuaHook(void* luaAlloc, void* ud) {
        lua_State* luaState = ogLuaHook(luaAlloc, ud);
        config::luastate = luaState;
        // spdlog::default_logger()->info("Hooked new LUA state: {}", (void*)luaState);
        return luaState;
}

void __stdcall detourReadPixels(void*, int x, int y, int width, int height, unsigned char* data, ImageFormat dstFormat) {
	ogReadPixels(interfaces::matrenderctx, x, y, width, height, data, dstFormat);
}

void __stdcall detourReadPixelsAndStretch(void*, void* pSrcRect, void* pDstRect, unsigned char* pBuffer, ImageFormat dstFormat, int nDstStride) {
	ogReadPixelsAndStretch(interfaces::matrenderctx, pSrcRect, pDstRect, pBuffer, dstFormat, nDstStride);
}

void __stdcall detourDrawModelExecute(void*, DrawModelState_t& state, ModelRenderInfo_t& pInfo, Matrix3x4* pCustomBoneToWorld = NULL) {
	if (config::esp_enabled && config::chams_enabled && pInfo.pModel) {
        std::string modelName = pInfo.pModel->name;
        bool is_player = pInfo.entity_index > 0 && pInfo.entity_index <= interfaces::engine->GetMaxClients();
        bool is_local_player = pInfo.entity_index == interfaces::engine->GetLocalPlayer();
        
        bool is_hand = modelName.find("arms") != std::string::npos;
        bool is_weapon = modelName.find("weapons/v_") != std::string::npos;

        bool apply_chams = false;
        
        if (is_player && !is_local_player) {
            apply_chams = true;
            
            // Glow implementation
            if (config::comp_glow.enabled) {
                IMaterial* glowMat = interfaces::matsystem->FindMaterial("debug/debugdrawflat", TEXTURE_GROUP_MODEL);
                if (glowMat) {
                    interfaces::modelrender->SuppressEngineLighting(true);
                    glowMat->ColorModulate(config::comp_glow.color[0], config::comp_glow.color[1], config::comp_glow.color[2]);
                    glowMat->AlphaModulate(config::comp_glow.color[3]);
                    interfaces::modelrender->ForcedMaterialOverride(glowMat, 0);
                    ogDrawModelExecute(interfaces::modelrender, state, pInfo, pCustomBoneToWorld);
                    interfaces::modelrender->ForcedMaterialOverride(nullptr, 0);
                    interfaces::modelrender->SuppressEngineLighting(false);
                }
            }
        } else if (config::hand_chams && (is_hand || is_weapon)) {
            apply_chams = true;
        }

        if (apply_chams && config::chams_mat != 0) { 
            bool should_chams = false;
            IMaterial* chamsMat = nullptr;

            if (config::chams_mat == 4) { // Wireframe
                chamsMat = interfaces::matsystem->FindMaterial("models/wireframe", TEXTURE_GROUP_MODEL);
                should_chams = true;
            } else if (config::chams_mat == 1) { // Flat
                chamsMat = interfaces::matsystem->FindMaterial("debug/debugdrawflat", TEXTURE_GROUP_MODEL);
                should_chams = true;
            } else if (config::chams_mat == 2) { // Material
                chamsMat = interfaces::matsystem->FindMaterial("debug/debugambientcube", TEXTURE_GROUP_MODEL);
                should_chams = true;
            }

            if (should_chams && chamsMat) {
                if (!is_hand && !is_weapon) {
                    chamsMat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
                } else {
                    chamsMat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
                }
                
                interfaces::modelrender->SuppressEngineLighting(true);
                chamsMat->ColorModulate(config::comp_box2d.color[0], config::comp_box2d.color[1], config::comp_box2d.color[2]);
                chamsMat->AlphaModulate(config::comp_box2d.color[3]);
                interfaces::modelrender->ForcedMaterialOverride(chamsMat, 0);
                
                ogDrawModelExecute(interfaces::modelrender, state, pInfo, pCustomBoneToWorld);
                
                interfaces::modelrender->ForcedMaterialOverride(nullptr, 0);
                interfaces::modelrender->SuppressEngineLighting(false);
                return; // Prevent drawing twice
            }
		}
	}
	ogDrawModelExecute(interfaces::modelrender, state, pInfo, pCustomBoneToWorld);
}

void hooks::Init() noexcept
{
        MH_CreateHook(
                mem::Get(interfaces::clientMode, 21),
                &detourCreateMove,
                reinterpret_cast<void**>(&ogCreateMove)
        );

        MH_CreateHook(
                reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("lua_shared.dll"), "lua_newstate")),
                &detourLuaHook,
                reinterpret_cast<void**>(&ogLuaHook)
        );

        MH_CreateHook(
                mem::Get(interfaces::modelrender, 20),
                &detourDrawModelExecute,
                reinterpret_cast<void**>(&ogDrawModelExecute)
        );

        MH_EnableHook(MH_ALL_HOOKS);
}
void hooks::Shutdown() noexcept
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
}
