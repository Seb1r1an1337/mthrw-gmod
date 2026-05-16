#pragma once
#include "../includes.hpp"
#include "cbaseentity.hpp"

enum Bones : int {
	bone_head = 6,
	bone_neck = 5,
	bone_spine_1 = 4,
	bone_spine_2 = 2,
	bone_pelvis = 0,
	bone_arm_top_l = 8,
	bone_arm_top_r = 13,
	bone_arm_bot_l = 9,
	bone_arm_bot_r = 14,
	bone_hand_l = 10,
	bone_hand_r = 15,
	bone_leg_top_l = 22,
	bone_leg_top_r = 25,
	bone_leg_bot_l = 23,
	bone_leg_bot_r = 26,
	bone_ANKLE_l = 24,
	bone_ANKLE_r = 27,
};

class C_BasePlayer : public C_BaseEntity {
public:
	int GetHealth() noexcept {
		return *reinterpret_cast<int*>(uintptr_t(this) + 0xC8);
	}

    int GetMaxHealth() noexcept {
        int max_hp = *reinterpret_cast<int*>(uintptr_t(this) + 0xCC);
        if (max_hp <= 0 || max_hp > 100000) return 100;
        return max_hp;
    }

	int GetFlags() noexcept {
		return *reinterpret_cast<int*>(uintptr_t(this) + 0x370); 
	}

	const Vector& GetAbsOrigin() noexcept {
		return mem::Call<const Vector&>(this, 9);
	}

	bool IsAlive() noexcept {
		return mem::Call<bool>(this, 129);
	}

	bool IsPlayer() noexcept {
		return mem::Call<bool>(this, 130);
	}

	const Vector& GetViewOffset() noexcept {
		return *reinterpret_cast<Vector*>(uintptr_t(this) + 0x108);
	}

	const Vector& GetVelocity() noexcept {
		return *reinterpret_cast<Vector*>(uintptr_t(this) + 0x94);
	}

	bool SetupBones(Matrix3x4* out, int max, int mask, float time) noexcept {
        void* pRenderable = (void*)(reinterpret_cast<uintptr_t>(this) + 0x8);
		return mem::Call<bool>(pRenderable, 13, out, max, mask, time);
	}

	void SetMaterialOverridePointer(void* mat) noexcept {
		mem::Call<void>(this, 161, mat);
	}

	bool IsDormant(void) noexcept {
        void* pNetworkable = (void*)(reinterpret_cast<uintptr_t>(this) + 0x10);
		return mem::Call<bool>(pNetworkable, 8);
	}

	Vector GetEyePosition() noexcept {
		return GetAbsOrigin() + GetViewOffset();
	}

	int GetTickBase() noexcept {
		return *reinterpret_cast<int*>(uintptr_t(this) + 0x2D48);
	}

	const Angle& GetViewPunch() noexcept {
		return *reinterpret_cast<Angle*>(uintptr_t(this) + 0x2A00);
	}

	const Angle& GetAimPunch() noexcept {
		return *reinterpret_cast<Angle*>(uintptr_t(this) + 0x250C);
	}

	const void* GetActiveWeapon() noexcept {
		return mem::Call<void*>(this, 280);
	}

    int GetMoveType() noexcept {
        return *reinterpret_cast<int*>(uintptr_t(this) + 0x25C);
    }

    void PushEntity() noexcept {
		mem::Call<void>(this, 172);
	}
};

inline C_BasePlayer* localPlayer = nullptr;