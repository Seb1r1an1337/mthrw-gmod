# MTH (X64)

A specialized C++ internal for Garry's Mod, built for the x64 architecture.
Cheat Base - [Leclerc-GMOD](https://github.com/sh1rm4n/leclerc-gmod)

## Features

### Combat
- **Aimbot**: Advanced aiming assistance with Silent Aim and Visible-only checks.
- **Triggerbot**: Automatic shooting when hovering over enemies with adjustable delay.
- **Accuracy**: No Recoil and No Spread compensation.
- **Backtrack**: Compensate for latency by targeting previous entity positions (up to 12 ticks).
- **Resolver**: Correction for anti-aim/jittery movements.
- **Customization**: 
  - Adjustable FOV (Field of View) with visual circle.
  - Smoothing and humanization for legit-looking gameplay.
  - Target selection (FOV, Distance, etc.).
  - Detailed filters: Ignore Friends, Team, NPCs, Dormant, or Invisible entities.

### Visuals (ESP)
- **Box ESP**: Multiple styles including 2D Flat, Bounding, Corners, and 3D Boxes.
- **Detailed Info**:
  - Name, Health (Bar & Percentage), Armor Bar.
  - Current Weapon, User Group (Rank), Job (DarkRP).
  - Distance, Snaplines, and Skeleton visualization.
  - Head Dot and Flags.
- **Chams**: Customizable player and hand chams with various materials.
- **Glow**: Outline entities for better visibility through walls.
- **Radar**: 2D radar for enhanced situational awareness.
- **Misc Visuals**:
  - Custom Crosshair and Recoil Crosshair.
  - Hitmarkers and Damage Logs.
  - Grenade Prediction.
  - Offscreen indicators.

### Movement
- **Bunnyhop**: Automatic jumping for maximum speed (Standard and Safe modes).
- **Autostrafe**: Legit and Rage modes for effortless air control.

### Lua Integration
- **Lua Loader**: Execute custom scripts within the game environment.
- **Lua API**: A dedicated `mth` namespace for scripting:
  - `mth.is_friend(index)`: Check if a player is marked as a friend.
  - `mth.is_visible(entity)`: Visibility check for entities.
  - `mth.get_crosshair_ent()`: Returns the entity currently under the crosshair.
  - `mth.get_screen_size()`: Returns current window resolution.
  - `mth.is_in_game()`: Check connection status.
  - `mth.get_local_player_name()`: Returns the name of the local player.
  - `mth.set_cache(index, is_player, info)`: Manually update entity cache data.
  - `mth.notify(text)`: Display a notification in the internal UI.
  - `mth.run_lua(script)`: Execute a Lua string.
  - `mth.is_menu_open()`: Check if the internal menu is active.
  - `mth.get_entity_by_class(class)`: Find an entity by its class name.
  - `mth.draw_line`, `mth.draw_box`, `mth.draw_text`: Custom drawing functions.

### Miscellaneous
- **Anti-Screenshot**: Protects against game-engine-based screen captures.
- **Chat Tools**: Customizable Chat Spam and Kill Say.
- **Bypasses**: Allow CS Lua and Allow Cheats integration.
- **Utility Windows**: Spectator list, Console, and Lua Executor.

## Libraries Used
- [ImGui](https://github.com/ocornut/imgui) - Bloat-free Graphical User interface for C++ with minimal dependencies.
- [spdlog](https://github.com/gabime/spdlog) - Very fast, header-only/compiled, C++ logging library.
- [MinHook](https://github.com/TsudaKageyu/minhook) - The Minimalistic x86/x64 Inline Hooking Library for Windows.
- [Kiero](https://github.com/Rebane2001/kiero) - Universal graphical hook for a variety of D3D versions.
- [FreeType](https://freetype.org/) - A freely available software library to render fonts.
- [Boost.Regex](https://www.boost.org/doc/libs/release/libs/regex/) - Powerful regular expression library.

## Disclaimer
This project is for educational purposes only. Use it at your own risk.

## License
[MIT License](LICENSE)
