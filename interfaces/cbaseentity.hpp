#pragma once
#include "mathlib.hpp"

struct ClientClass {
    void* m_pCreateFn;
    void* m_pCreateEventFn;
    const char* m_pNetworkName;
    void* m_pRecvTable;
    ClientClass* m_pNext;
    int m_ClassID;
};

class IClientNetworkable {
public:
    virtual void* GetIClientUnknown() = 0;
    virtual void Release() = 0;
    virtual ClientClass* GetClientClass() = 0;
};

class ICollideable {
public:
    const Vector& OBBMins() { return mem::Call<const Vector&>(this, 1); }
    const Vector& OBBMaxs() { return mem::Call<const Vector&>(this, 2); }
};

class C_BaseEntity {
public:
    IClientNetworkable* GetNetworkable() {
        // x64 offset for IClientNetworkable is 0x10 (16 bytes)
        return reinterpret_cast<IClientNetworkable*>(reinterpret_cast<uintptr_t>(this) + 0x10);
    }

    ICollideable* GetCollideable() {
        return mem::Call<ICollideable*>(this, 3);
    }

    ClientClass* GetClientClass() {
        IClientNetworkable* networkable = GetNetworkable();
        if (!networkable) return nullptr;
        
        // Ensure VTable is valid (VTable is at beginning of the networkable pointer)
        void** vtable = *(void***)networkable;
        if (!vtable) return nullptr;

        // GetClientClass is index 2
        typedef ClientClass* (__thiscall* OriginalFn)(void*);
        return ((OriginalFn)vtable[2])(networkable);
    }
    
    const Vector& GetAbsOrigin() {
        return mem::Call<const Vector&>(this, 9);
    }

    const Vector& GetRenderOrigin() {
        // x64 offset for IClientRenderable is 0x8
        void* pRenderable = (void*)(reinterpret_cast<uintptr_t>(this) + 0x8);
        if (!pRenderable) return GetAbsOrigin();
        
        void** vtable = *(void***)pRenderable;
        if (!vtable) return GetAbsOrigin();

        // GetRenderOrigin is index 1
        typedef const Vector& (__thiscall* OriginalFn)(void*);
        return ((OriginalFn)vtable[1])(pRenderable);
    }
    
    const int& GetHealth() { 
        return *reinterpret_cast<int*>(uintptr_t(this) + 0xC8); 
    }

    const int& GetMaxHealth() {
        return *reinterpret_cast<int*>(uintptr_t(this) + 0xCC);
    }
    
    bool IsDormant() { 
        IClientNetworkable* networkable = GetNetworkable();
        if (!networkable) return true;
        void** vtable = *(void***)networkable;
        if (!vtable) return true;
        
        // IsDormant is index 8
        typedef bool (__thiscall* OriginalFn)(void*);
        return ((OriginalFn)vtable[8])(networkable);
    }

    void PushEntity() {
        // PushEntity virtual index on x64 is 183 for GMod (was 172 in x86)
        mem::Call<void>(this, 183);
    }
};
