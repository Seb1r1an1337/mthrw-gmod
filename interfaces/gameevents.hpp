#pragma once
#include "../includes.hpp"
// https://wiki.facepunch.com/gmod/Game_Events
class DamageEvent : IGameEventListener2
{
public:
	DamageEvent(void) {};
	~DamageEvent(void) {};
	void FireGameEvent(IGameEvent* event) override
	{
		if (!event)
			return;
		int localPlayerID = interfaces::engine->GetLocalPlayer();
		int target = interfaces::engine->GetPlayerForUserID(event->GetInt("userid")); // UserID of the victim
		int attacker = interfaces::engine->GetPlayerForUserID(event->GetInt("attacker")); // UserID of the attacker

		player_info_s targetInfo;
		player_info_s attackerInfo;
		interfaces::engine->GetPlayerInfo(target, &targetInfo);
		interfaces::engine->GetPlayerInfo(attacker, &attackerInfo);

		if (strlen(attackerInfo.name) && attacker != localPlayerID)
			//std::cout << attackerInfo.name << " attacked " << targetInfo.name << ". NEW HP: " << event->GetInt("health") << std::endl;
			spdlog::default_logger()->info(std::format("{} attacked {}", attackerInfo.name, targetInfo.name));

		if (target == localPlayerID || attacker != localPlayerID)
			return;

		//if (Settings::Misc::hitmarkerSoundEnabled)
		//	MatSystemSurface->PlaySound(hitMarkers[Settings::Misc::hitmarkerSound]);

		//Settings::lastHitmarkerTime = EngineClient->Time();
	}
};

class KillSayEvent : public IGameEventListener2
{
public:
    void FireGameEvent(IGameEvent* event) override
    {
        if (!event || !config::kill_say_enabled || config::kill_say_list.empty())
            return;

        if (strcmp(event->GetName(), "player_death") != 0)
            return;

        int local_index = interfaces::engine->GetLocalPlayer();
        int victim_index = interfaces::engine->GetPlayerForUserID(event->GetInt("userid"));
        int attacker_index = interfaces::engine->GetPlayerForUserID(event->GetInt("attacker"));

        // Debug log to console
        spdlog::info("[KillSay] Event caught! Name: {}", event->GetName());

        if (victim_index != local_index) {
            static int say_idx = 0;
            if (say_idx >= (int)config::kill_say_list.size()) say_idx = 0;

            std::string msg = config::kill_say_list[say_idx];
            std::string cmd = config::chat_spam_ooc ? "say /ooc " + msg : "say " + msg;

            interfaces::engine->ClientCmd_Unrestricted(cmd.c_str());
            spdlog::info("[KillSay] Sending (victim died): {}", cmd);
            say_idx++;
        }
    }

    int GetEventDebugID(void) { return 42; }
};

inline KillSayEvent killSayEvent;