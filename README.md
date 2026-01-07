# CoDUO_QOL - A plugin for Call of Duty: United Offensive
Aiming to provide enhancements, quality of life features and extended features for CoDUO SP & MP with the main focus being SP


<p align="center">
  <img src="https://github.com/user-attachments/assets/ada7ac00-201e-42ca-9b0d-5293c90a1af6" 
       alt="Highlights"/><br>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/6c71a533-7836-42e6-b948-2b27dad6d32e" 
       alt="cg_fixedAspect"/><br><br>
</p>

<h5 id="cg_weaponsprint_mod"></h5>

<p align="center">
  <img src="https://github.com/user-attachments/assets/dc0ccdae-9d7a-4c06-8941-c7535c0b1771" 
       alt="cg_weaponsprint_mod"/>
</p>






## Features\cvars:
- `cg_fixedAspect` - Fixes HUD stretching, not recommended to be changed live requires vid_restart for some elements.
- Cinematics now properly display above 4:3 instead of showing as a black screen.
- `r_fixedaspect_clear` - clears color and depth buffers before drawing, 1 always clears it, while 2 only clears it while in "UI",menus & cinematics
- `cg_fixedAspectFOV` - Implements HOR+ FOV scaling when set to 1
- `safeArea_horizontal` - Horizontal safe area as a fraction of the screen width, might not effect all HUD elements, setting this to 0.0 will cause the HUD to be in 4:3 borders
- `safeArea_vertical` - not fully implemented.
- `cg_fovscale` - applies a multipler to overall FOV
- `cg_fovscale_ads` - applies a multipler to ADS FOV
- `m_rawinput` - toggles raw mouse input.
- `cg_fovMin`
- `r_qol_texture_filter_anisotropic` determines anisotropic filtering, only if GPU vendor supports `GL_EXT_texture_filter_anisotropic`
- `r_noborder` - Borderless windowed when `r_fullscreen` 0 is set to 0, `r_fullscreen 0;r_noborder 1;vid_ypos 0;vid_xpos 0;vid_restart` is recommended
- `r_mode_auto` - when set to 1, it'll automatically determine your screen's resolution when `r_mode` is set to -1
- `player_sprintSpeedScale` - Scales sprint speed. (SP only)
- `player_sprintmult` - controls how fast the sprint meter will run out. (SP only)
- `g_enemyFireDist` - distance before tagging client's crosshair red, 0 disables this functionality (SP only)
- `cg_drawCrosshair_friendly_green` - 1 will colour the crosshair to green when aiming at friendly NPCs (SP only)
- `cg_weaponSprint_mod` - restores <sup><sup>yes this was planned for UO</sup></sup>  and implements new [weapon sprinting bob and rotation attributes](#cg_weaponsprint_mod) inspired by T4, requires new eWeapon files for each weapon (SP only)
- `reload_eweapons` - command to reload eWeapon definitions (SP only)
- and possible some more, use the `qol_showallcvars` command to to show all cvars registered by the plugin!

## Installation
1. Download latest [release](https://github.com/Clippy95/CoDUO_QOL/releases/latest) or for semi-experimental builds use [actions](https://github.com/Clippy95/CoDUO_QOL/actions)
2. Extract contents to your game folder
3. Launch the game

### Installation notes
CoDUO_QOL can also be loaded using [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), by renaming the .dll extension to an .asi (name can also be changed), but this requires setting DontLoadFromDllMain=0 in UAL's global.ini for Steam versions.

### Credits:
- [RTCW-SP/MP](https://github.com/id-Software/RTCW-SP/)
- [ioq3](https://github.com/ioquake/ioq3)
- [AlphaYellowWidescreenFixes](https://github.com/alphayellow1/AlphaYellowWidescreenFixes) for the x86 FPU operations wrappers
- [iw1x-client](https://github.com/hBrutal/iw1x-client)
- [ModUtils](https://github.com/CookiePLMonster/ModUtils/tree/master)
- [safetyhook](https://github.com/cursey/safetyhook)
- [VladWinner](https://github.com/VladWinner) - For tesing, suggestions and other several contributions.
- [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
- momo5502

  
