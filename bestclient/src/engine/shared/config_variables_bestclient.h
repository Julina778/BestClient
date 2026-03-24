// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Tcme, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Tcme, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Tcme, ScriptName, Len, Def, Save, Desc) ;
#endif

#if defined(CONF_FAMILY_WINDOWS)
MACRO_CONFIG_INT(TcAllowAnyRes, tc_allow_any_res, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Whether to allow any resolution in game when zoom is allowed (buggy on Windows)")
#else
MACRO_CONFIG_INT(TcAllowAnyRes, tc_allow_any_res, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Whether to allow any resolution in game when zoom is allowed (buggy on Windows)")
#endif

MACRO_CONFIG_INT(TcShowChatClient, tc_show_chat_client, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show chat messages from the client such as echo")

MACRO_CONFIG_INT(TcShowFrozenText, tc_frozen_tees_text, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show how many tees in your team are currently frozen. (0 - off, 1 - show alive, 2 - show frozen)")
MACRO_CONFIG_INT(TcShowFrozenHud, tc_frozen_tees_hud, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show frozen tee HUD")
MACRO_CONFIG_INT(TcShowFrozenHudSkins, tc_frozen_tees_hud_skins, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use ninja skin, or darkened skin for frozen tees on hud")

MACRO_CONFIG_INT(TcFrozenHudTeeSize, tc_frozen_tees_size, 15, 8, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of tees in frozen tee hud. (Default : 15)")
MACRO_CONFIG_INT(TcFrozenMaxRows, tc_frozen_tees_max_rows, 1, 1, 6, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum number of rows in frozen tee HUD display")
MACRO_CONFIG_INT(TcFrozenHudTeamOnly, tc_frozen_tees_only_inteam, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only render frozen tee HUD display while in team")

MACRO_CONFIG_INT(TcNameplatePingCircle, tc_nameplate_ping_circle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows a circle to indicate ping in the nameplate")
MACRO_CONFIG_INT(BcNameplateAsyncIndicator, bc_nameplate_async_indicator, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows an async indicator from 1/5 to 5/5 in your nameplate")
MACRO_CONFIG_INT(TcNameplateCountry, tc_nameplate_country, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows the country flag in the nameplate")
MACRO_CONFIG_INT(TcNameplateSkins, tc_nameplate_skins, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows skin names in nameplates, good for finding missing skins")
MACRO_CONFIG_INT(BcNameplateCountryOffsetX, bc_nameplate_country_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the country flag in nameplates")
MACRO_CONFIG_INT(BcNameplateCountryOffsetY, bc_nameplate_country_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the country flag in nameplates")
MACRO_CONFIG_INT(BcNameplatePingOffsetX, bc_nameplate_ping_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the ping indicator in nameplates")
MACRO_CONFIG_INT(BcNameplatePingOffsetY, bc_nameplate_ping_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the ping indicator in nameplates")
MACRO_CONFIG_INT(BcNameplateAsyncOffsetX, bc_nameplate_async_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the async indicator in nameplates")
MACRO_CONFIG_INT(BcNameplateAsyncOffsetY, bc_nameplate_async_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the async indicator in nameplates")
MACRO_CONFIG_INT(BcNameplateIgnoreOffsetX, bc_nameplate_ignore_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the ignore marker in nameplates")
MACRO_CONFIG_INT(BcNameplateIgnoreOffsetY, bc_nameplate_ignore_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the ignore marker in nameplates")
MACRO_CONFIG_INT(BcNameplateFriendOffsetX, bc_nameplate_friend_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the friend marker in nameplates")
MACRO_CONFIG_INT(BcNameplateFriendOffsetY, bc_nameplate_friend_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the friend marker in nameplates")
MACRO_CONFIG_INT(BcNameplateClientIdInlineOffsetX, bc_nameplate_client_id_inline_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the inline client id in nameplates")
MACRO_CONFIG_INT(BcNameplateClientIdInlineOffsetY, bc_nameplate_client_id_inline_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the inline client id in nameplates")
MACRO_CONFIG_INT(BcNameplateNameOffsetX, bc_nameplate_name_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the player name in nameplates")
MACRO_CONFIG_INT(BcNameplateNameOffsetY, bc_nameplate_name_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the player name in nameplates")
MACRO_CONFIG_INT(BcNameplateVoiceOffsetX, bc_nameplate_voice_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the voice icon in nameplates")
MACRO_CONFIG_INT(BcNameplateVoiceOffsetY, bc_nameplate_voice_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the voice icon in nameplates")
MACRO_CONFIG_INT(BcNameplateClientIndicatorOffsetX, bc_nameplate_client_indicator_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the client indicator in nameplates")
MACRO_CONFIG_INT(BcNameplateClientIndicatorOffsetY, bc_nameplate_client_indicator_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the client indicator in nameplates")
MACRO_CONFIG_INT(BcNameplateClanOffsetX, bc_nameplate_clan_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the clan line in nameplates")
MACRO_CONFIG_INT(BcNameplateClanOffsetY, bc_nameplate_clan_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the clan line in nameplates")
MACRO_CONFIG_INT(BcNameplateReasonOffsetX, bc_nameplate_reason_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the reason line in nameplates")
MACRO_CONFIG_INT(BcNameplateReasonOffsetY, bc_nameplate_reason_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the reason line in nameplates")
MACRO_CONFIG_INT(BcNameplateSkinOffsetX, bc_nameplate_skin_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the skin line in nameplates")
MACRO_CONFIG_INT(BcNameplateSkinOffsetY, bc_nameplate_skin_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the skin line in nameplates")
MACRO_CONFIG_INT(BcNameplateClientIdSeparateOffsetX, bc_nameplate_client_id_separate_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the separate client id line in nameplates")
MACRO_CONFIG_INT(BcNameplateClientIdSeparateOffsetY, bc_nameplate_client_id_separate_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the separate client id line in nameplates")
MACRO_CONFIG_INT(BcNameplateHookIconOffsetX, bc_nameplate_hook_icon_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the hook strength icon in nameplates")
MACRO_CONFIG_INT(BcNameplateHookIconOffsetY, bc_nameplate_hook_icon_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the hook strength icon in nameplates")
MACRO_CONFIG_INT(BcNameplateHookIdOffsetX, bc_nameplate_hook_id_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the hook strength id in nameplates")
MACRO_CONFIG_INT(BcNameplateHookIdOffsetY, bc_nameplate_hook_id_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the hook strength id in nameplates")
MACRO_CONFIG_INT(BcNameplateDirLeftOffsetX, bc_nameplate_dir_left_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the left direction icon in nameplates")
MACRO_CONFIG_INT(BcNameplateDirLeftOffsetY, bc_nameplate_dir_left_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the left direction icon in nameplates")
MACRO_CONFIG_INT(BcNameplateDirUpOffsetX, bc_nameplate_dir_up_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the up direction icon in nameplates")
MACRO_CONFIG_INT(BcNameplateDirUpOffsetY, bc_nameplate_dir_up_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the up direction icon in nameplates")
MACRO_CONFIG_INT(BcNameplateDirRightOffsetX, bc_nameplate_dir_right_offset_x, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal offset for the right direction icon in nameplates")
MACRO_CONFIG_INT(BcNameplateDirRightOffsetY, bc_nameplate_dir_right_offset_y, 0, -400, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical offset for the right direction icon in nameplates")

MACRO_CONFIG_INT(TcFakeCtfFlags, tc_fake_ctf_flags, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows fake CTF flags on people (0 = off, 1 = red, 2 = blue)")

MACRO_CONFIG_INT(TcLimitMouseToScreen, tc_limit_mouse_to_screen, 1, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Limit mouse to screen boundaries")
MACRO_CONFIG_INT(TcScaleMouseDistance, tc_scale_mouse_distance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Improve mouse precision by scaling max distance to 1000")

MACRO_CONFIG_INT(TcHammerRotatesWithCursor, tc_hammer_rotates_with_cursor, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Allow your hammer to rotate like other weapons")

MACRO_CONFIG_INT(TcMiniVoteHud, tc_mini_vote_hud, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use mini vote HUD instead of normal vote HUD")

//Controls
MACRO_CONFIG_INT(BCPrevMouseMaxDistance45degrees, bc_prev_mouse_max_distance_45_degrees, 400, 0, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_INSENSITIVE, "Previous maximum cursor distance for 45 degrees")
MACRO_CONFIG_INT(BCPrevInpMousesens45degrees, bc_prev_inp_mousesens_45_degrees, 200, 1, 100000, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Previous mouse sensitivity for 45 degrees")
MACRO_CONFIG_INT(BCToggle45degrees, bc_toggle_45_degrees, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle 45 degrees bind or not")
MACRO_CONFIG_INT(BCPrevInpMousesensSmallsens, bc_prev_inp_mousesens_small_sens, 200, 1, 100000, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Previous mouse sensitivity for small sens")
MACRO_CONFIG_INT(BCToggleSmallSens, bc_toggle_small_sens, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle small sens bind or not")
MACRO_CONFIG_INT(BcGoresMode, bc_gores_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Entity-like gores mode without bind")
MACRO_CONFIG_INT(BcGoresModeDisableIfWeapons, bc_gores_mode_disable_weapons, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Disable gores mode when holding shotgun, grenade or laser")
MACRO_CONFIG_INT(BcCrystalLaser, bc_crystal_laser, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render rifle and shotgun lasers with crystal shards and icy glow")
MACRO_CONFIG_INT(BcHookCombo, bc_hook_combo, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show hook combo popups with combo sounds")
MACRO_CONFIG_INT(BcHookComboMode, bc_hook_combo_mode, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hook combo trigger mode (0=hook, 1=hammer, 2=hook and hammer)")
MACRO_CONFIG_INT(BcHookComboResetTime, bc_hook_combo_reset_time, 1200, 100, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum time in ms between player hooks before combo restarts")
MACRO_CONFIG_INT(BcHookComboSoundVolume, bc_hook_combo_sound_volume, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hook combo sound volume")
MACRO_CONFIG_INT(BcHookComboSize, bc_hook_combo_size, 100, 50, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hook combo popup size")
MACRO_CONFIG_INT(BcMusicPlayer, bc_music_player, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable Music Player HUD element")
MACRO_CONFIG_INT(BcMusicPlayerShowWhenPaused, bc_music_player_show_when_paused, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Keep Music Player visible while playback is paused")
MACRO_CONFIG_INT(BcMusicPlayerColorMode, bc_music_player_color_mode, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Music player color mode (0=static color, 1=cover blur color)")
MACRO_CONFIG_COL(BcMusicPlayerStaticColor, bc_music_player_static_color, 128, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Static color for the music player when static color mode is selected")
MACRO_CONFIG_INT(BcDisabledComponentsMaskLo, bc_disabled_components_mask_lo, 0, 0, 2147483647, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Low bitmask for disabled BestClient components")
MACRO_CONFIG_INT(BcDisabledComponentsMaskHi, bc_disabled_components_mask_hi, 0, 0, 2147483647, CFGFLAG_CLIENT | CFGFLAG_SAVE, "High bitmask for disabled BestClient components")

	// Cava HUD visualizer (system audio capture)
	MACRO_CONFIG_INT(BcCavaEnable, bc_cava_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable Cava HUD visualizer (system output via loopback/monitor)")
	MACRO_CONFIG_INT(BcCavaCaptureDevice, bc_cava_capture_device, -1, -1, 64, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava capture device index (-1=auto)")
	MACRO_CONFIG_INT(BcCavaDockBottom, bc_cava_dock_bottom, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Dock Cava visualizer to bottom edge and stretch full width")
	MACRO_CONFIG_INT(BcCavaX, bc_cava_x, 250, 0, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava HUD position X in canvas units (0..500)")
	MACRO_CONFIG_INT(BcCavaY, bc_cava_y, 236, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava HUD position Y in canvas units (0..300)")
MACRO_CONFIG_INT(BcCavaW, bc_cava_w, 160, 20, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava HUD width in canvas units (0..500)")
MACRO_CONFIG_INT(BcCavaH, bc_cava_h, 48, 10, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava HUD height in canvas units (0..200)")
MACRO_CONFIG_INT(BcCavaBars, bc_cava_bars, 40, 4, 128, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Number of Cava frequency bars")
MACRO_CONFIG_INT(BcCavaFftSize, bc_cava_fft_size, 2048, 1024, 4096, CFGFLAG_CLIENT | CFGFLAG_SAVE, "FFT size for Cava (1024/2048/4096)")
MACRO_CONFIG_INT(BcCavaLowHz, bc_cava_low_hz, 50, 20, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Low cutoff frequency in Hz for Cava")
MACRO_CONFIG_INT(BcCavaHighHz, bc_cava_high_hz, 16000, 1000, 20000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "High cutoff frequency in Hz for Cava")
MACRO_CONFIG_INT(BcCavaGain, bc_cava_gain, 120, 50, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava gain in percent")
MACRO_CONFIG_INT(BcCavaAttack, bc_cava_attack, 60, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava bar attack speed (1..100)")
MACRO_CONFIG_INT(BcCavaDecay, bc_cava_decay, 40, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cava bar decay speed (1..100)")
MACRO_CONFIG_COL(BcCavaColor, bc_cava_color, 0xFF968C80U, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Bar color for Cava visualizer")
MACRO_CONFIG_INT(BcCavaBackground, bc_cava_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draw background behind Cava visualizer")
MACRO_CONFIG_COL(BcCavaBackgroundColor, bc_cava_background_color, 0x66000000U, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Background color for Cava visualizer")
MACRO_CONFIG_INT(BcMenuSfx, bc_menu_sfx, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable osu!lazer menu sound effects")
MACRO_CONFIG_INT(BcMenuSfxVolume, bc_menu_sfx_volume, 70, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "osu!lazer menu sound effects volume")
MACRO_CONFIG_INT(BcMenuMediaBackground, bc_menu_media_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom media background in offline menus")
MACRO_CONFIG_INT(BcGameMediaBackground, bc_game_media_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom media background in game background rendering")
MACRO_CONFIG_STR(BcMenuMediaBackgroundPath, bc_menu_media_background_path, IO_MAX_PATH_LENGTH, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Path to the custom menu media background file")
MACRO_CONFIG_INT(BcGameMediaBackgroundOffset, bc_game_media_background_offset, 0, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How much the custom media background is fixed to the map when rendering the in-game background")
MACRO_CONFIG_INT(BcShopAutoSet, bc_shop_auto_set, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply downloaded shop assets automatically")

// Optimizer
MACRO_CONFIG_INT(BcOptimizer, bc_optimizer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable optimizer features")
MACRO_CONFIG_INT(BcOptimizerDisableParticles, bc_optimizer_disable_particles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Disable rendering/updating all particles")
MACRO_CONFIG_INT(BcOptimizerFpsFog, bc_optimizer_fps_fog, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cull non-map rendering outside a distance limit around the camera")
MACRO_CONFIG_INT(BcOptimizerDdnetPriorityHigh, bc_optimizer_ddnet_priority_high, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Set DDNet process priority to High while enabled")
MACRO_CONFIG_INT(BcOptimizerDiscordPriorityBelowNormal, bc_optimizer_discord_priority_below_normal, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Set Discord process priority to Below Normal while enabled")
MACRO_CONFIG_INT(BcOptimizerFpsFogMode, bc_optimizer_fps_fog_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "FPS fog mode (0=manual radius tiles, 1=by zoom percent)")
MACRO_CONFIG_INT(BcOptimizerFpsFogRadiusTiles, bc_optimizer_fps_fog_radius_tiles, 40, 5, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "FPS fog manual radius in tiles (tile=32 units)")
MACRO_CONFIG_INT(BcOptimizerFpsFogZoomPercent, bc_optimizer_fps_fog_zoom_percent, 70, 10, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "FPS fog visible area percent in zoom mode")
MACRO_CONFIG_INT(BcOptimizerFpsFogRenderRect, bc_optimizer_fps_fog_render_rect, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render an outline rectangle showing the FPS fog area")
MACRO_CONFIG_INT(BcOptimizerFpsFogCullMapTiles, bc_optimizer_fps_fog_cull_map_tiles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cull map tile rendering outside the FPS fog area")

// Focus Mode Settings
MACRO_CONFIG_INT(ClFocusMode, p_focus_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable focus mode to minimize visual distractions")
MACRO_CONFIG_INT(ClFocusModeHideNames, p_focus_mode_hide_names, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide player names in focus mode")
MACRO_CONFIG_INT(ClFocusModeHideEffects, p_focus_mode_hide_effects, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide visual effects in focus mode")
MACRO_CONFIG_INT(ClFocusModeHideHud, p_focus_mode_hide_hud, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide HUD in focus mode")
MACRO_CONFIG_INT(ClFocusModeHideSongPlayer, p_focus_mode_hide_song_player, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide song player in focus mode")
MACRO_CONFIG_INT(ClFocusModeHideUI, p_focus_mode_hide_ui, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide unnecessary UI elements in focus mode")
MACRO_CONFIG_INT(ClFocusModeHideChat, p_focus_mode_hide_chat, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide chat in focus mode")
MACRO_CONFIG_INT(ClFocusModeHideScoreboard, p_focus_mode_hide_scoreboard, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide scoreboard in focus mode")
//Effects controls
MACRO_CONFIG_INT(ClFreezeSnowFlakes, p_effect_freeze_snowflakes, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "toggles snowflakes effect")
MACRO_CONFIG_INT(ClHammerHitEffect, p_effect_hammerhit, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "toggles hammer hit effect")
MACRO_CONFIG_INT(ClHammerHitEffectSound, p_effect_sound_hammerhit, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "toggles hammer hit effect")
MACRO_CONFIG_INT(ClJumpEffect, p_effect_jump, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "toggles hammer hit effect")
MACRO_CONFIG_INT(ClJumpEffectSound, p_effect_sound_jump, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "toggles jump effect sound")

// Anti Latency Tools
MACRO_CONFIG_INT(TcRemoveAnti, tc_remove_anti, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Removes some amount of antiping & player prediction in freeze")
MACRO_CONFIG_INT(TcUnfreezeLagTicks, tc_remove_anti_ticks, 5, 0, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "The biggest amount of prediction ticks that are removed")
MACRO_CONFIG_INT(TcUnfreezeLagDelayTicks, tc_remove_anti_delay_ticks, 25, 5, 150, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many ticks it takes to remove the maximum prediction after being frozen")

MACRO_CONFIG_INT(TcUnpredOthersInFreeze, tc_unpred_others_in_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Dont predict other players if you are frozen")
MACRO_CONFIG_INT(TcPredMarginInFreeze, tc_pred_margin_in_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable changing prediction margin while frozen")
MACRO_CONFIG_INT(TcPredMarginInFreezeAmount, tc_pred_margin_in_freeze_amount, 15, 0, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Set what your prediction margin while frozen should be")

MACRO_CONFIG_INT(TcShowOthersGhosts, tc_show_others_ghosts, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show ghosts for other players in their unpredicted position")
MACRO_CONFIG_INT(TcSwapGhosts, tc_swap_ghosts, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show predicted players as ghost and normal players as unpredicted")
MACRO_CONFIG_INT(TcHideFrozenGhosts, tc_hide_frozen_ghosts, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide Ghosts of other players if they are frozen")

MACRO_CONFIG_INT(TcPredGhostsAlpha, tc_pred_ghosts_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha of predicted ghosts (0-100)")
MACRO_CONFIG_INT(TcUnpredGhostsAlpha, tc_unpred_ghosts_alpha, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha of unpredicted ghosts (0-100)")
MACRO_CONFIG_INT(TcRenderGhostAsCircle, tc_render_ghost_as_circle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render Ghosts as circles instead of tee")

MACRO_CONFIG_INT(TcShowCenter, tc_show_center, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws lines to show the center of your screen/hitbox")
MACRO_CONFIG_INT(TcShowCenterWidth, tc_show_center_width, 0, 0, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Center lines width (enabled by tc_show_center)")
MACRO_CONFIG_COL(TcShowCenterColor, tc_show_center_color, 1694498688, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Center lines color (enabled by tc_show_center)") // transparent red

MACRO_CONFIG_INT(TcFastInput, tc_fast_input, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Uses input for prediction before the next tick")
MACRO_CONFIG_INT(BcFastInputMode, bc_fast_input_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fast input mode (0 = fast input, 1 = low delta)")
MACRO_CONFIG_INT(TcFastInputAmount, tc_fast_input_amount, 20, 0, 40, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many milliseconds fast input will apply")
MACRO_CONFIG_INT(BcFastInputLowDelta, bc_fast_input_low_delta, 0, 0, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fast input amount for low delta mode in 0.01 ticks (0-5)")
MACRO_CONFIG_INT(TcFastInputOthers, tc_fast_input_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply fast input to other tees")
MACRO_CONFIG_INT(BcLowDeltaOthers, bc_low_delta_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply low delta to other tees")

MACRO_CONFIG_INT(TcAntiPingImproved, tc_antiping_improved, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Different antiping smoothing algorithm, not compatible with cl_antiping_smooth")
MACRO_CONFIG_INT(TcAntiPingNegativeBuffer, tc_antiping_negative_buffer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Helps in Gores. Allows internal certainty value to be negative which causes more conservative prediction")
MACRO_CONFIG_INT(TcAntiPingStableDirection, tc_antiping_stable_direction, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Predicts optimistically along the tees stable axis to reduce delay in gaining overall stability")
MACRO_CONFIG_INT(TcAntiPingUncertaintyScale, tc_antiping_uncertainty_scale, 150, 25, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Determines uncertainty duration as a factor of ping, 100 = 1.0")

MACRO_CONFIG_INT(TcColorFreeze, tc_color_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use skin colors for frozen tees")
MACRO_CONFIG_INT(TcColorFreezeDarken, tc_color_freeze_darken, 90, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Makes color of tees darker when in freeze (0-100)")
MACRO_CONFIG_INT(TcColorFreezeFeet, tc_color_freeze_feet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Also use color for frozen tee feet")
MACRO_CONFIG_INT(BcEmoticonShadow, bc_emoticon_shadow, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draw shadow behind emoticons")
MACRO_CONFIG_INT(BcChatSaveDraft, bc_chat_save_draft, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Keep unfinished chat input when closing chat")

// Revert Variables
MACRO_CONFIG_INT(TcSmoothPredictionMargin, tc_prediction_margin_smooth, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Makes prediction margin transition smooth, causes worse ping jitter adjustment (reverts a DDNet change)")
MACRO_CONFIG_INT(TcFrozenKatana, tc_frozen_katana, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show katana on frozen players (reverts a DDNet change)")
MACRO_CONFIG_INT(TcOldTeamColors, tc_old_team_colors, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use rainbow team colors (reverts a DDNet change)")

// Outline Variables
MACRO_CONFIG_INT(TcOutline, tc_outline, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws outlines")
MACRO_CONFIG_INT(TcOutlineEntities, tc_outline_in_entities, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only show outlines in entities")

MACRO_CONFIG_INT(TcOutlineSolid, tc_outline_solid, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws outline around hook and unhook")
MACRO_CONFIG_INT(TcOutlineFreeze, tc_outline_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws outline around freeze and deep")
MACRO_CONFIG_INT(TcOutlineUnfreeze, tc_outline_unfreeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws outline around unfreeze and undeep")
MACRO_CONFIG_INT(TcOutlineKill, tc_outline_kill, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws outline around kill")
MACRO_CONFIG_INT(TcOutlineTele, tc_outline_tele, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Draws outline around teleporters")

MACRO_CONFIG_INT(TcOutlineWidthSolid, tc_outline_width_solid, 2, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Width of outline around hook and unhook")
MACRO_CONFIG_INT(TcOutlineWidthFreeze, tc_outline_width_freeze, 2, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Width of outline around freeze and deep")
MACRO_CONFIG_INT(TcOutlineWidthUnfreeze, tc_outline_width_unfreeze, 2, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Width of outline around unfreeze and undeep")
MACRO_CONFIG_INT(TcOutlineWidthKill, tc_outline_width_kill, 2, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Width of outline around kill")
MACRO_CONFIG_INT(TcOutlineWidthTele, tc_outline_width_tele, 2, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Width of outline around teleporters")

MACRO_CONFIG_COL(TcOutlineColorSolid, tc_outline_color_solid, 4294901760, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of outline around hook and unhook") // 255 0 0 0
MACRO_CONFIG_COL(TcOutlineColorFreeze, tc_outline_color_freeze, 4294901760, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of outline around freeze and deep") // 255 0 0 0
MACRO_CONFIG_COL(TcOutlineColorUnfreeze, tc_outline_color_unfreeze, 4294901760, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of outline around unfreeze and undeep") // 255 0 0 0
MACRO_CONFIG_COL(TcOutlineColorKill, tc_outline_color_kill, 4294901760, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of outline around kill") // 0 0 0
MACRO_CONFIG_COL(TcOutlineColorTele, tc_outline_color_tele, 4294901760, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of outline around teleporters") // 255 0 0 0

// Indicator Variables
MACRO_CONFIG_COL(TcIndicatorAlive, tc_indicator_alive, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color of alive tees in player indicator")
MACRO_CONFIG_COL(TcIndicatorFreeze, tc_indicator_freeze, 65407, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color of frozen tees in player indicator")
MACRO_CONFIG_COL(TcIndicatorSaved, tc_indicator_dead, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color of tees who is getting saved in player indicator")
MACRO_CONFIG_INT(TcIndicatorOffset, tc_indicator_offset, 42, 16, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "(16-128) Offset of indicator position")
MACRO_CONFIG_INT(TcIndicatorOffsetMax, tc_indicator_offset_max, 100, 16, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "(16-128) Max indicator offset for variable offset setting")
MACRO_CONFIG_INT(TcIndicatorVariableDistance, tc_indicator_variable_distance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Indicator circles will be further away the further the tee is")
MACRO_CONFIG_INT(TcIndicatorMaxDistance, tc_indicator_variable_max_distance, 1000, 500, 7000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum tee distance for variable offset")
MACRO_CONFIG_INT(TcIndicatorRadius, tc_indicator_radius, 4, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "(1-16) indicator circle size")
MACRO_CONFIG_INT(TcIndicatorOpacity, tc_indicator_opacity, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Opacity of indicator circles")
MACRO_CONFIG_INT(TcPlayerIndicator, tc_player_indicator, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show radial indicator of other tees")
MACRO_CONFIG_INT(TcPlayerIndicatorFreeze, tc_player_indicator_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only show frozen tees in indicator")
MACRO_CONFIG_INT(TcIndicatorTeamOnly, tc_indicator_inteam, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only show indicator while in team")
MACRO_CONFIG_INT(TcIndicatorTees, tc_indicator_tees, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show tees instead of circles")
MACRO_CONFIG_INT(TcIndicatorHideVisible, tc_indicator_hide_visible_tees, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Don't show tees that are on your screen")

// Regex chat matching
MACRO_CONFIG_STR(TcRegexChatIgnore, tc_regex_chat_ignore, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Filters out chat messages based on a regular expression.")

// Misc visual
MACRO_CONFIG_INT(TcWhiteFeet, tc_white_feet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render all feet as perfectly white base color")
MACRO_CONFIG_STR(TcWhiteFeetSkin, tc_white_feet_skin, 255, "x_ninja", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Base skin for white feet")
MACRO_CONFIG_INT(TcRenderWeaponsAsGun, tc_render_weapons_as_gun, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Renders weapons as the gun sprite instead of the weapon, with the exception of hammer and ninja (1 = with hue, 2 = without hue)")
MACRO_CONFIG_INT(TcMovingTilesEntities, tc_moving_tiles_entities, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show moving tiles in entities")

MACRO_CONFIG_INT(TcMiniDebug, tc_mini_debug, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show position and angle")

MACRO_CONFIG_INT(TcNotifyWhenLast, tc_last_notify, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Notify when you are last")
MACRO_CONFIG_STR(TcNotifyWhenLastText, tc_last_notify_text, 64, "Last!", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Text for last notify")
MACRO_CONFIG_COL(TcNotifyWhenLastColor, tc_last_notify_color, 256, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color for last notify")
MACRO_CONFIG_INT(TcNotifyWhenLastX, tc_last_notify_x, 20, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Horizontal position for last notify as percentage of screen width")
MACRO_CONFIG_INT(TcNotifyWhenLastY, tc_last_notify_y, 1, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Vertical position for last notify as percentage of screen height")
MACRO_CONFIG_INT(TcNotifyWhenLastSize, tc_last_notify_size, 10, 0, 50, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Font size for last notify")

MACRO_CONFIG_INT(TcRenderCursorSpec, tc_cursor_in_spec, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render your gun cursor when spectating in freeview")
MACRO_CONFIG_INT(TcRenderCursorSpecAlpha, tc_cursor_in_spec_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha of cursor in freeview")

// MACRO_CONFIG_INT(TcRenderNameplateSpec, tc_render_nameplate_spec, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render nameplates when spectating")

MACRO_CONFIG_INT(TcTinyTees, tc_tiny_tees, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render tees smaller")
MACRO_CONFIG_INT(TcTinyTeeSize, tc_indicator_tees_size, 100, 85, 115, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Define the Size of the Tiny Tee")
MACRO_CONFIG_INT(TcTinyTeesOthers, tc_tiny_tees_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render other tees smaller")

MACRO_CONFIG_INT(TcCursorScale, tc_cursor_scale, 100, 0, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Percentage to scale the in game cursor by as a percentage (50 = half, 200 = double)")

// Profiles
MACRO_CONFIG_INT(TcProfileSkin, tc_profile_skin, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply skin in profiles")
MACRO_CONFIG_INT(TcProfileName, tc_profile_name, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply name in profiles")
MACRO_CONFIG_INT(TcProfileClan, tc_profile_clan, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply clan in profiles")
MACRO_CONFIG_INT(TcProfileFlag, tc_profile_flag, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply flag in profiles")
MACRO_CONFIG_INT(TcProfileColors, tc_profile_colors, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply colors in profiles")
MACRO_CONFIG_INT(TcProfileEmote, tc_profile_emote, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Apply emote in profiles")
MACRO_CONFIG_INT(TcProfileOverwriteClanWithEmpty, tc_profile_overwrite_clan_with_empty, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Overwrite clan name even if profile has an empty clan name")

// Rainbow
MACRO_CONFIG_INT(TcRainbowTees, tc_rainbow_tees, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Turn on rainbow client side")
MACRO_CONFIG_INT(TcRainbowHook, tc_rainbow_hook, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow hook")
MACRO_CONFIG_INT(TcRainbowWeapon, tc_rainbow_weapon, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow Weapons")

MACRO_CONFIG_INT(TcRainbowOthers, tc_rainbow_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Turn on rainbow client side for others")
MACRO_CONFIG_INT(TcRainbowMode, tc_rainbow_mode, 1, 1, 4, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow mode (1: rainbow, 2: pulse, 3: darkness, 4: random)")
MACRO_CONFIG_INT(TcRainbowSpeed, tc_rainbow_speed, 100, 1, 10000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow speed as a percentage (50 = half speed, 200 = double speed)")

// War List
MACRO_CONFIG_INT(TcWarList, tc_warlist, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggles war list visuals")
MACRO_CONFIG_INT(TcWarListShowClan, tc_warlist_show_clan_if_war, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show clan in nameplate if there is a war")
MACRO_CONFIG_INT(TcWarListReason, tc_warlist_reason, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show war reason")
MACRO_CONFIG_INT(TcWarListChat, tc_warlist_chat, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show war colors in chat")
MACRO_CONFIG_INT(TcWarListScoreboard, tc_warlist_scoreboard, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show war colors in scoreboard")
MACRO_CONFIG_INT(TcWarListAllowDuplicates, tc_warlist_allow_duplicates, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Allow duplicate war entries")
MACRO_CONFIG_INT(TcWarListSpectate, tc_warlist_spectate, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show war colors in spectator menu")

MACRO_CONFIG_INT(TcWarListIndicator, tc_warlist_indicator, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use warlist for indicator")
MACRO_CONFIG_INT(TcWarListIndicatorColors, tc_warlist_indicator_colors, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show warlist colors instead of freeze colors")
MACRO_CONFIG_INT(TcWarListIndicatorAll, tc_warlist_indicator_all, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show all groups")
MACRO_CONFIG_INT(TcWarListIndicatorEnemy, tc_warlist_indicator_enemy, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show players from the first group")
MACRO_CONFIG_INT(TcWarListIndicatorTeam, tc_warlist_indicator_team, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show players from second group")

// Status Bar
MACRO_CONFIG_INT(TcStatusBar, tc_statusbar, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable status bar")

MACRO_CONFIG_INT(TcStatusBar12HourClock, tc_statusbar_12_hour_clock, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use 12 hour clock in local time")
MACRO_CONFIG_INT(TcStatusBarLocalTimeSeocnds, tc_statusbar_local_time_seconds, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show seconds in local time")
MACRO_CONFIG_INT(TcStatusBarHeight, tc_statusbar_height, 8, 1, 16, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Height of the status bar")

MACRO_CONFIG_COL(TcStatusBarColor, tc_statusbar_color, 3221225472, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Status bar background color")
MACRO_CONFIG_COL(TcStatusBarTextColor, tc_statusbar_text_color, 4278190335, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Status bar text color")
MACRO_CONFIG_INT(TcStatusBarAlpha, tc_statusbar_alpha, 75, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Status bar background alpha")
MACRO_CONFIG_INT(TcStatusBarTextAlpha, tc_statusbar_text_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Status bar text alpha")

MACRO_CONFIG_INT(TcStatusBarLabels, tc_statusbar_labels, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show labels on status bar entries")
MACRO_CONFIG_STR(TcStatusBarScheme, tc_statusbar_scheme, 128, "ac pf r", CFGFLAG_CLIENT | CFGFLAG_SAVE, "The order in which to show status bar items")

// Trails
MACRO_CONFIG_INT(TcTeeTrail, tc_tee_trail, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable Tee trails")
MACRO_CONFIG_INT(TcTeeTrailOthers, tc_tee_trail_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show tee trails for other players")
MACRO_CONFIG_INT(TcTeeTrailWidth, tc_tee_trail_width, 15, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Tee trail width")
MACRO_CONFIG_INT(TcTeeTrailLength, tc_tee_trail_length, 25, 5, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Tee trail length")
MACRO_CONFIG_INT(TcTeeTrailAlpha, tc_tee_trail_alpha, 80, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Tee trail alpha")
MACRO_CONFIG_COL(TcTeeTrailColor, tc_tee_trail_color, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Tee trail color")
MACRO_CONFIG_INT(TcTeeTrailTaper, tc_tee_trail_taper, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Taper tee trail over length")
MACRO_CONFIG_INT(TcTeeTrailFade, tc_tee_trail_fade, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fade trail alpha over length")
MACRO_CONFIG_INT(TcTeeTrailColorMode, tc_tee_trail_color_mode, 1, 1, 5, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Tee trail color mode (1: Solid color, 2: Current Tee color, 3: Rainbow, 4: Color based on Tee speed, 5: Random)")

// Chat Reply
MACRO_CONFIG_INT(TcAutoReplyMuted, tc_auto_reply_muted, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto reply to muted players with a message")
MACRO_CONFIG_STR(TcAutoReplyMutedMessage, tc_auto_reply_muted_message, 128, "I have muted you", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message to reply to muted players")
MACRO_CONFIG_INT(TcAutoReplyMinimized, tc_auto_reply_minimized, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto reply when your game is minimized")
MACRO_CONFIG_STR(TcAutoReplyMinimizedMessage, tc_auto_reply_minimized_message, 128, "I am not tabbed in", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message to reply when your game is minimized")

// Voting
MACRO_CONFIG_INT(TcAutoVoteWhenFar, tc_auto_vote_when_far, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto vote no if you far on a map")
MACRO_CONFIG_STR(TcAutoVoteWhenFarMessage, tc_auto_vote_when_far_message, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message to send when auto far vote happens, leave empty to disable")
MACRO_CONFIG_INT(TcAutoVoteWhenFarTime, tc_auto_vote_when_far_time, 5, 0, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long until auto vote far happens")
MACRO_CONFIG_INT(BcAutoTeamLock, bc_auto_team_lock, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically lock your team after joining it")
MACRO_CONFIG_INT(BcAutoTeamLockDelay, bc_auto_team_lock_delay, 5, 0, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Delay before auto-locking team after joining, in seconds")
MACRO_CONFIG_INT(BcAutoDummyJoin, bc_auto_dummy_join, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically sync main and dummy into the same race team")

// Font
MACRO_CONFIG_STR(TcCustomFont, tc_custom_font, 255, "DejaVu Sans", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Custom font face")

// Bg Draw
MACRO_CONFIG_INT(TcBgDraw, tc_bg_draw, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable background draw")
MACRO_CONFIG_INT(TcBgDrawWidth, tc_bg_draw_width, 5, 1, 50, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Width of background draw strokes")
MACRO_CONFIG_INT(TcBgDrawFadeTime, tc_bg_draw_fade_time, 0, 0, 600, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Time until strokes disappear (0 = never)")
MACRO_CONFIG_INT(TcBgDrawMaxItems, tc_bg_draw_max_items, 128, 0, 2048, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum number of strokes")
MACRO_CONFIG_COL(TcBgDrawColor, tc_bg_draw_color, 14024576, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color of background draw strokes")
MACRO_CONFIG_INT(TcBgDrawAutoSaveLoad, tc_bg_draw_auto_save_load, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically save and load background drawings")

// Translate
MACRO_CONFIG_STR(TcTranslateBackend, tc_translate_backend, 32, "google", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Translate backends (google, ftapi, libretranslate)")
MACRO_CONFIG_STR(BcTranslateIncomingSource, bc_translate_incoming_source, 16, "auto", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Source language for incoming chat translation (use auto to detect)")
MACRO_CONFIG_STR(TcTranslateTarget, tc_translate_target, 16, "auto", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Target language for incoming chat translation")
MACRO_CONFIG_STR(BcTranslateOutgoingSource, bc_translate_outgoing_source, 16, "auto", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Source language for your outgoing chat translation (use auto to detect)")
MACRO_CONFIG_STR(BcTranslateOutgoingTarget, bc_translate_outgoing_target, 16, "auto", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Target language for your outgoing chat translation")
MACRO_CONFIG_STR(TcTranslateEndpoint, tc_translate_endpoint, 256, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "For backends which need it, endpoint to use (must be https)")
MACRO_CONFIG_STR(TcTranslateKey, tc_translate_key, 256, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "For backends which need it, api key to use")
MACRO_CONFIG_INT(TcTranslateAutoIncoming, tc_translate_auto_incoming, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable automatic translation of incoming chat messages (other players)")
MACRO_CONFIG_INT(TcTranslateAutoOutgoing, tc_translate_auto_outgoing, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable automatic translation of your outgoing chat messages")
MACRO_CONFIG_INT(TcTranslateAuto, tc_translate_auto, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "DEPRECATED (migrates on startup): use tc_translate_auto_incoming and tc_translate_auto_outgoing")

// Animations
MACRO_CONFIG_INT(TcAnimateWheelTime, tc_animate_wheel_time, 150, 1, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Duration of emote and BindSystem animations, in milliseconds")

// Pets
MACRO_CONFIG_INT(TcPetShow, tc_pet_show, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show a pet")
MACRO_CONFIG_STR(TcPetSkin, tc_pet_skin, 24, "twinbop", CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_INSENSITIVE, "Pet skin")
MACRO_CONFIG_INT(TcPetSize, tc_pet_size, 60, 10, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of the pet as a percentage of a normal player")
MACRO_CONFIG_INT(TcPetAlpha, tc_pet_alpha, 90, 10, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha of pet (100 = fully opaque, 50 = half transparent)")

// Change name near finish
MACRO_CONFIG_INT(TcChangeNameNearFinish, tc_change_name_near_finish, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Attempt to change your name when near finish")
MACRO_CONFIG_STR(TcFinishName, tc_finish_name, 16, "nameless tee", CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_INSENSITIVE, "Name to change to when near finish when tc_change_name_near_finish is 1")

// Flags
MACRO_CONFIG_INT(BcBestClientSettingsTabs, bc_bestclient_settings_tabs, 0, 0, 65536, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Bit flags to disable settings tabs")
MACRO_CONFIG_INT(BcSettingsLayout, bc_settings_layout, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Menu layout (0 = new, 1 = old)")

// Volleyball
MACRO_CONFIG_INT(TcVolleyBallBetterBall, tc_volleyball_better_ball, 1, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Make frozen players in volleyball look more like volleyballs (0 = disabled, 1 = in volleyball maps, 2 = always)")
MACRO_CONFIG_STR(TcVolleyBallBetterBallSkin, tc_volleyball_better_ball_skin, 24, "beachball", CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_INSENSITIVE, "Player skin to use for better volleyball ball")

// Mod
MACRO_CONFIG_INT(TcShowPlayerHitBoxes, tc_show_player_hit_boxes, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show player hit boxes (1 = predicted, 2 = predicted and unpredicted)")
MACRO_CONFIG_INT(TcHideChatBubbles, tc_hide_chat_bubbles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide your own chat bubbles, only works when authed in remote console")
MACRO_CONFIG_INT(TcModWeapon, tc_mod_weapon, 0, 0, 1, CFGFLAG_CLIENT, "Run a command (default kill) when you point and shoot at someone, only works when authed in remote console")
MACRO_CONFIG_STR(TcModWeaponCommand, tc_mod_weapon_command, 256, "rcon kill_pl", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Command to run with tc_mod_weapon, id is appended to end of command")

// Run on join
MACRO_CONFIG_STR(TcExecuteOnConnect, tc_execute_on_connect, 100, "Run a console command before connect", CFGFLAG_CLIENT | CFGFLAG_SAVE, "")
MACRO_CONFIG_STR(TcExecuteOnJoin, tc_execute_on_join, 100, "Run a console command on join", CFGFLAG_CLIENT | CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(TcExecuteOnJoinDelay, tc_execute_on_join_delay, 2, 7, 50000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Tick delay before executing tc_execute_on_join")

// Custom Communities
MACRO_CONFIG_STR(TcCustomCommunitiesUrl, tc_custom_communities_url, 256, "https://raw.githubusercontent.com/SollyBunny/ddnet-custom-communities/refs/heads/main/custom-communities-ddnet-info.json", CFGFLAG_CLIENT | CFGFLAG_SAVE, "URL to fetch custom communities from (must be https), empty to disable")

// Discord RPC
MACRO_CONFIG_INT(ClDiscordRPC, ec_discord_rpc, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle Discord Rpc (no restart needed)")
MACRO_CONFIG_INT(ClDiscordMapStatus, ec_discord_map_status, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show What Map you're on")

MACRO_CONFIG_INT(TcShowLocalTimeSeconds, tc_show_local_time_seconds, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show local time in seconds")

// Configs tab UI
MACRO_CONFIG_INT(TcUiShowDDNet, tc_ui_show_ddnet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show DDNet domain in Configs tab")
MACRO_CONFIG_INT(BcUiShowBestClient, bc_ui_show_bestclient, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show BestClient domain in Configs tab")
MACRO_CONFIG_INT(TcUiOnlyModified, tc_ui_only_modified, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show only modified settings in Configs tab")
MACRO_CONFIG_INT(TcUiCompactList, tc_ui_compact_list, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use compact row layout in Configs tab")

// Dummy Info
MACRO_CONFIG_INT(TcShowhudDummyPosition, tc_showhud_dummy_position, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show ingame HUD (Dummy Position)")
MACRO_CONFIG_INT(TcShowhudDummySpeed, tc_showhud_dummy_speed, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show ingame HUD (Dummy Speed)")
MACRO_CONFIG_INT(TcShowhudDummyAngle, tc_showhud_dummy_angle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show ingame HUD (Dummy Aim Angle)")
MACRO_CONFIG_INT(BcShowhudDummyCoordIndicator, bc_showhud_dummy_coord_indicator, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show player-below indicator in ingame HUD")
MACRO_CONFIG_COL(BcShowhudDummyCoordIndicatorColor, bc_showhud_dummy_coord_indicator_color, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Player-below indicator color")
MACRO_CONFIG_COL(BcShowhudDummyCoordIndicatorSameHeightColor, bc_showhud_dummy_coord_indicator_same_height_color, 65407, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Player-below indicator color when aligned vertically")


// Best Client
MACRO_CONFIG_INT(BcLoadscreenToggle, bc_loadscreentoggle, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle loading screen on/off")

// Vusials

// Magic Particles
MACRO_CONFIG_INT(BcMagicParticles, bc_magic_particles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle magic particles")
MACRO_CONFIG_INT(BcMagicParticlesRadius, bc_magic_particles_radius, 10, 1, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Radius of magic particles")
MACRO_CONFIG_INT(BcMagicParticlesSize, bc_magic_particles_size, 8, 1, 50, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of magic particles")
MACRO_CONFIG_INT(BcMagicParticlesAlphaDelay, bc_magic_particles_alpha_delay, 3, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha delay of magic particles")
MACRO_CONFIG_INT(BcMagicParticlesType, bc_magic_particles_type, 1, 1, 4, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Type of magic particles. 1 = SLICE, 2 = BALL, 3 = SMOKE, 4 = SHELL")
MACRO_CONFIG_INT(BcMagicParticlesCount, bc_magic_particles_count, 10, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Count of magic particles")


// i'll add animations later
// Animations
MACRO_CONFIG_INT(BcAnimations, bc_animations, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle animations")
MACRO_CONFIG_INT(BcModuleUiRevealAnimation, bc_module_ui_reveal_animation, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle module settings reveal animations")
MACRO_CONFIG_INT(BcModuleUiRevealAnimationMs, bc_module_ui_reveal_animation_ms, 180, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Module settings reveal time (in ms)")
MACRO_CONFIG_INT(BcIngameMenuAnimation, bc_ingame_menu_animation, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle ingame ESC menu animation")
MACRO_CONFIG_INT(BcIngameMenuAnimationMs, bc_ingame_menu_animation_ms, 220, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Ingame ESC menu animation time (in ms)")

// Chat Animation
MACRO_CONFIG_INT(BcChatAnimation, bc_chat_animation, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle chat animation")
MACRO_CONFIG_INT(BcChatAnimationMs, bc_chat_animation_ms, 300, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Chat animation time (in ms)")
MACRO_CONFIG_INT(BcChatOpenAnimation, bc_chat_open_animation, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle chat open animation")
MACRO_CONFIG_INT(BcChatOpenAnimationMs, bc_chat_open_animation_ms, 220, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Chat open animation time (in ms)")
MACRO_CONFIG_INT(BcChatTypingAnimation, bc_chat_typing_animation, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle chat typing animation")
MACRO_CONFIG_INT(BcChatTypingAnimationMs, bc_chat_typing_animation_ms, 180, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Chat typing animation time (in ms)")
MACRO_CONFIG_INT(BcChatAnimationType, bc_chat_animation_type, 3, 1, 4, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Chat animation type")

// Chat Bubbles
MACRO_CONFIG_INT(BcChatBubbles, bc_chat_bubbles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle Chatbubbles")
MACRO_CONFIG_INT(BcChatBubblesSelf, bc_chat_bubbles_self, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles above you")
MACRO_CONFIG_INT(BcChatBubblesDemo, bc_chat_bubbles_demo, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles in demoplayer")
MACRO_CONFIG_INT(BcChatBubbleSize, bc_chat_bubble_size, 20, 20, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of the chat bubble")
MACRO_CONFIG_INT(BcChatBubbleShowTime, bc_chat_bubble_showtime, 200, 200, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long to show the bubble for")
MACRO_CONFIG_INT(BcChatBubbleFadeOut, bc_chat_bubble_fadeout, 35, 15, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long it fades out")
MACRO_CONFIG_INT(BcChatBubbleFadeIn, bc_chat_bubble_fadein, 15, 15, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long it fades in")

// Client Indicator
MACRO_CONFIG_INT(BcClientIndicator, bc_client_indicator, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle client indicator")
MACRO_CONFIG_INT(DbgClientIndicator, dbg_client_indicator, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Debug logging for BestClient indicator (1=verbose, 2=dump all UDP packet bytes sent/received)")

MACRO_CONFIG_INT(BcClientIndicatorInNamePlate, bc_client_indicator_in_name_plate, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show client indicator in name plate")
MACRO_CONFIG_INT(BcClientIndicatorInNamePlateAboveSelf, bc_client_indicator_in_name_plate_above_self, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show client indicator above self")
MACRO_CONFIG_INT(BcClientIndicatorInNamePlateSize, bc_client_indicator_in_name_plate_size, 30, -50, 100, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Client indicator in name plate size")
MACRO_CONFIG_INT(BcClientIndicatorInNamePlateDynamic, bc_client_indicator_in_name_plate_dynamic, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Client indicator in nameplates will dynamically change pos")

MACRO_CONFIG_INT(BcClientIndicatorInScoreboard, bc_client_indicator_in_scoreboard, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show client indicator in name plate")
MACRO_CONFIG_INT(BcClientIndicatorInSoreboardSize, bc_client_indicator_in_scoreboard_size, 30, -50, 100, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Client indicator in name plate size")

MACRO_CONFIG_STR(BcClientIndicatorServerAddress, bc_client_indicator_server_address, 256, "150.241.70.188:8778", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Client indicator UDP presence server")
MACRO_CONFIG_STR(BcClientIndicatorBrowserUrl, bc_client_indicator_browser_url, 256, "https://150.241.70.188:8779/users.json", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Client indicator browser JSON URL")
MACRO_CONFIG_STR(BcClientIndicatorTokenUrl, bc_client_indicator_token_url, 256, "https://150.241.70.188:8779/token.json", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Client indicator token bootstrap URL")
MACRO_CONFIG_STR(BcClientIndicatorSharedToken, bc_client_indicator_shared_token, 256, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Client indicator shared token for signed UDP packets")
MACRO_CONFIG_INT(BrFilterBestclient, br_filter_bestclient, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Filter out servers with no BestClient users")

// Last Notify sound
MACRO_CONFIG_INT(BcNotifyWhenLastSound, bc_notify_when_last_sound, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Play sound if you are last")

// Camera drift
MACRO_CONFIG_INT(BcCameraDrift, bc_camera_drift, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable smooth camera drift that lags behind player movement")  
MACRO_CONFIG_INT(BcCameraDriftAmount, bc_camera_drift_amount, 50, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Amount of camera drift (1 = minimal drift, 200 = maximum drift)")  
MACRO_CONFIG_INT(BcCameraDriftSmoothness, bc_camera_drift_smoothness, 20, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Smoothness of camera drift (1 = near instant, 20 = very smooth)")
MACRO_CONFIG_INT(BcCameraDriftReverse, bc_camera_drift_reverse, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Reverse camera drift direction (camera drifts opposite to movement)")
MACRO_CONFIG_INT(BcDynamicFov, bc_dynamic_fov, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Increase FOV dynamically based on movement speed")
MACRO_CONFIG_INT(BcDynamicFovAmount, bc_dynamic_fov_amount, 50, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Amount of dynamic FOV boost (1 = minimal boost, 200 = maximum boost)")
MACRO_CONFIG_INT(BcDynamicFovSmoothness, bc_dynamic_fov_smoothness, 20, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Smoothness of dynamic FOV boost (1 = near instant, 20 = very smooth)")
MACRO_CONFIG_INT(BcCinematicCamera, bc_cinematic_camera, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable smooth cinematic camera movement in spectator freeview")

// Afterimage
MACRO_CONFIG_INT(BcAfterimage, bc_afterimage, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render afterimage layers of your tee")
MACRO_CONFIG_INT(BcAfterimageFrames, bc_afterimage_frames, 6, 2, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many previous frames to keep for the afterimage")
MACRO_CONFIG_INT(BcAfterimageAlpha, bc_afterimage_alpha, 40, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum alpha of afterimage layers (1-100)")
MACRO_CONFIG_INT(BcAfterimageSpacing, bc_afterimage_spacing, 18, 1, 64, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Distance between afterimage samples")

// Speedrun Timer
MACRO_CONFIG_INT(BcSpeedrunTimer, bc_speedrun_timer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speedrun timer")
MACRO_CONFIG_INT(BcSpeedrunTimerTime, bc_speedrun_timer_time, 0, 0, 9999, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speedrun timer time (MMSS format)")
MACRO_CONFIG_INT(BcSpeedrunTimerHours, bc_speedrun_timer_hours, 0, 0, 99, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speedrun timer hours")
MACRO_CONFIG_INT(BcSpeedrunTimerMinutes, bc_speedrun_timer_minutes, 0, 0, 59, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speedrun timer minutes")
MACRO_CONFIG_INT(BcSpeedrunTimerSeconds, bc_speedrun_timer_seconds, 0, 0, 59, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speedrun timer seconds")
MACRO_CONFIG_INT(BcSpeedrunTimerMilliseconds, bc_speedrun_timer_milliseconds, 0, 0, 999, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speedrun timer milliseconds")
MACRO_CONFIG_INT(BcSpeedrunTimerAutoDisable, bc_speedrun_timer_auto_disable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Disable speedrun timer automatically when time ends")

// Admin panel
MACRO_CONFIG_INT(BcAdminPanelAutoScroll, bc_adminpanel_autoscroll, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto-scroll logs in admin panel")
MACRO_CONFIG_INT(BcAdminPanelRememberTab, bc_adminpanel_remember_tab, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Remember last active admin panel tab")
MACRO_CONFIG_INT(BcAdminPanelLastTab, bc_adminpanel_last_tab, 0, 0, 10, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Last active admin panel tab")
MACRO_CONFIG_INT(BcAdminPanelDisableAnim, bc_adminpanel_disable_anim, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Disable admin panel animations")
MACRO_CONFIG_INT(BcAdminPanelScale, bc_adminpanel_scale, 100, 80, 120, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel scale in percent")
MACRO_CONFIG_INT(BcAdminPanelLogLines, bc_adminpanel_log_lines, 200, 50, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum lines to keep in admin panel logs")
MACRO_CONFIG_COL(BcAdminPanelBgColor, bc_adminpanel_bg_color, 0x8C000000, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Admin panel background color")
MACRO_CONFIG_COL(BcAdminPanelTabInactiveColor, bc_adminpanel_tab_inactive_color, 0xCC00002E, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Admin panel inactive tab color")
MACRO_CONFIG_COL(BcAdminPanelTabActiveColor, bc_adminpanel_tab_active_color, 0xE6000052, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Admin panel active tab color")
MACRO_CONFIG_COL(BcAdminPanelTabHoverColor, bc_adminpanel_tab_hover_color, 0xE600003D, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Admin panel hover tab color")
MACRO_CONFIG_STR(BcAdminFastAction0, bc_admin_fast_action0, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 1")
MACRO_CONFIG_STR(BcAdminFastAction1, bc_admin_fast_action1, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 2")
MACRO_CONFIG_STR(BcAdminFastAction2, bc_admin_fast_action2, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 3")
MACRO_CONFIG_STR(BcAdminFastAction3, bc_admin_fast_action3, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 4")
MACRO_CONFIG_STR(BcAdminFastAction4, bc_admin_fast_action4, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 5")
MACRO_CONFIG_STR(BcAdminFastAction5, bc_admin_fast_action5, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 6")
MACRO_CONFIG_STR(BcAdminFastAction6, bc_admin_fast_action6, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 7")
MACRO_CONFIG_STR(BcAdminFastAction7, bc_admin_fast_action7, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 8")
MACRO_CONFIG_STR(BcAdminFastAction8, bc_admin_fast_action8, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 9")
MACRO_CONFIG_STR(BcAdminFastAction9, bc_admin_fast_action9, 96, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Admin panel fast action command slot 10")

// Orbit Aura
MACRO_CONFIG_INT(BcOrbitAura, bc_orbit_aura, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle orbit aura around the local player")
MACRO_CONFIG_INT(BcOrbitAuraRadius, bc_orbit_aura_radius, 32, 8, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura radius")
MACRO_CONFIG_INT(BcOrbitAuraParticles, bc_orbit_aura_particles, 14, 2, 120, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura particle count")
MACRO_CONFIG_INT(BcOrbitAuraAlpha, bc_orbit_aura_alpha, 70, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura alpha")
MACRO_CONFIG_INT(BcOrbitAuraSpeed, bc_orbit_aura_speed, 100, 10, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura speed")
MACRO_CONFIG_INT(BcOrbitAuraIdle, bc_orbit_aura_idle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only show orbit aura after standing idle")
MACRO_CONFIG_INT(BcOrbitAuraIdleTimer, bc_orbit_aura_idle_timer, 2, 1, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Idle delay before orbit aura appears in seconds")

// 3D Particles
MACRO_CONFIG_INT(Bc3dParticles, bc_3d_particles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesType, bc_3d_particles_type, 1, 1, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Type of 3D particles. 1 = Cube, 2 = Heart, 3 = Mixed")
MACRO_CONFIG_INT(Bc3dParticlesCount, bc_3d_particles_count, 60, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Count of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesSizeMin, bc_3d_particles_size_min, 3, 2, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Minimum size of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesSizeMax, bc_3d_particles_size_max, 8, 2, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum size of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesSpeed, bc_3d_particles_speed, 18, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Base speed of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesDepth, bc_3d_particles_depth, 300, 10, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Depth range for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesAlpha, bc_3d_particles_alpha, 35, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha of 3D particles (1-100)")
MACRO_CONFIG_INT(Bc3dParticlesFadeInMs, bc_3d_particles_fade_in_ms, 400, 1, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fade-in time in ms for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesFadeOutMs, bc_3d_particles_fade_out_ms, 400, 1, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fade-out time in ms for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesPushRadius, bc_3d_particles_push_radius, 120, 0, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Player push radius for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesPushStrength, bc_3d_particles_push_strength, 120, 0, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Player push strength for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesCollide, bc_3d_particles_collide, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "3D particles collide with each other")
MACRO_CONFIG_INT(Bc3dParticlesViewMargin, bc_3d_particles_view_margin, 120, 0, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Margin outside view before 3D particles disappear")
MACRO_CONFIG_INT(Bc3dParticlesColorMode, bc_3d_particles_color_mode, 1, 1, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color mode for 3D particles. 1 = White, 2 = Random")
MACRO_CONFIG_COL(Bc3dParticlesColor, bc_3d_particles_color, 4294967295, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesGlow, bc_3d_particles_glow, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable 3D particles glow")
MACRO_CONFIG_INT(Bc3dParticlesGlowAlpha, bc_3d_particles_glow_alpha, 35, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Glow alpha of 3D particles (1-100)")
MACRO_CONFIG_INT(Bc3dParticlesGlowOffset, bc_3d_particles_glow_offset, 2, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Glow offset for 3D particles")

// Lock cam
MACRO_CONFIG_INT(BcLockCamUseCustomZoom, bc_lock_cam_use_custom_zoom, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Use custom zoom for lock cam")
MACRO_CONFIG_INT(BcLockCamZoom, bc_lock_cam_zoom, 85, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Lock cam zoom (percent)")

// Voice chat
MACRO_CONFIG_INT(BcVoiceChatEnable, bc_voice_chat_enable, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable integrated voice chat")
MACRO_CONFIG_INT(BcVoiceChatActivationMode, bc_voice_chat_activation_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Voice activation mode (0 = automatic activation, 1 = push-to-talk)")
MACRO_CONFIG_INT(BcVoiceChatVolume, bc_voice_chat_volume, 100, 0, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Voice playback volume in percent")
MACRO_CONFIG_INT(BcVoiceChatMicGain, bc_voice_chat_mic_gain, 100, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Microphone gain in percent")
MACRO_CONFIG_INT(BcVoiceChatBitrate, bc_voice_chat_bitrate, 96, 6, 96, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Voice Opus bitrate in kbps")
MACRO_CONFIG_INT(BcVoiceChatInputDevice, bc_voice_chat_input_device, -1, -1, 64, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Voice input device index (-1 = system default)")
MACRO_CONFIG_INT(BcVoiceChatOutputDevice, bc_voice_chat_output_device, -1, -1, 64, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Voice output device index (-1 = system default)")

// Debug
MACRO_CONFIG_INT(DbgCava, dbg_cava, 0, 0, 2, CFGFLAG_CLIENT, "Debug Cava/Audio visualizer (1=overlay, 2=overlay+logs)")
MACRO_CONFIG_INT(BcVoiceChatMicMuted, bc_voice_chat_mic_muted, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Mute voice microphone")
MACRO_CONFIG_INT(BcVoiceChatHeadphonesMuted, bc_voice_chat_headphones_muted, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Mute voice playback")
MACRO_CONFIG_INT(BcVoiceChatMicCheck, bc_voice_chat_mic_check, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable microphone check (local loopback)")
MACRO_CONFIG_INT(BcVoiceChatNameplateIcon, bc_voice_chat_nameplate_icon, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show microphone icon in name plates for talking players")
MACRO_CONFIG_STR(BcVoiceChatServerAddress, bc_voice_chat_server_address, 128, "127.0.0.1:8777", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Voice server address")
MACRO_CONFIG_STR(BcVoiceChatMutedNames, bc_voice_chat_muted_names, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Comma-separated list of muted voice nicknames (case-insensitive)")
MACRO_CONFIG_STR(BcVoiceChatNameVolumes, bc_voice_chat_name_volumes, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Comma-separated list of voice nickname volumes in percent (name=value, case-insensitive)")

// World editor
MACRO_CONFIG_INT(BcWorldEditor, bc_world_editor, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable World Editor post processing")
MACRO_CONFIG_INT(BcWorldEditorPanelX, bc_world_editor_panel_x, 70, 0, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor panel X position")
MACRO_CONFIG_INT(BcWorldEditorPanelY, bc_world_editor_panel_y, 70, 0, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor panel Y position")
MACRO_CONFIG_INT(BcWorldEditorMotionBlur, bc_world_editor_motion_blur, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable World Editor motion blur")
MACRO_CONFIG_INT(BcWorldEditorMotionBlurStrength, bc_world_editor_motion_blur_strength, 35, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor motion blur strength")
MACRO_CONFIG_INT(BcWorldEditorMotionBlurResponse, bc_world_editor_motion_blur_response, 65, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor motion blur response")
MACRO_CONFIG_INT(BcWorldEditorBloom, bc_world_editor_bloom, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable World Editor bloom")
MACRO_CONFIG_INT(BcWorldEditorBloomIntensity, bc_world_editor_bloom_intensity, 40, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor bloom intensity")
MACRO_CONFIG_INT(BcWorldEditorBloomThreshold, bc_world_editor_bloom_threshold, 55, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor bloom threshold")
MACRO_CONFIG_INT(BcWorldEditorOutline, bc_world_editor_outline, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable World Editor outline")
MACRO_CONFIG_INT(BcWorldEditorOutlineIntensity, bc_world_editor_outline_intensity, 45, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor outline intensity")
MACRO_CONFIG_INT(BcWorldEditorOutlineThreshold, bc_world_editor_outline_threshold, 30, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "World Editor outline threshold")

// Graphics
MACRO_CONFIG_INT(BcCustomAspectRatioMode, bc_custom_aspect_ratio_mode, -1, -1, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Aspect ratio mode (-1=legacy auto, 0=off, 1=preset, 2=custom)")
MACRO_CONFIG_INT(BcCustomAspectRatio, bc_custom_aspect_ratio, 0, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Aspect ratio value x100 (0=off, presets: 125=5:4, 133=4:3, 150=3:2, custom: 100-300)")

// Chat media
MACRO_CONFIG_INT(BcChatMediaPreview, bc_chat_media_preview, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render media previews from chat links")
MACRO_CONFIG_INT(BcChatMediaPhotos, bc_chat_media_photos, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render photo previews from chat links")
MACRO_CONFIG_INT(BcChatMediaGifs, bc_chat_media_gifs, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render GIF and animated media previews from chat links")
MACRO_CONFIG_INT(BcChatMediaPreviewMaxWidth, bc_chat_media_preview_max_width, 220, 120, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum width of chat media previews")
MACRO_CONFIG_INT(BcChatMediaViewer, bc_chat_media_viewer, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable fullscreen media viewer for chat previews")
MACRO_CONFIG_INT(BcChatMediaViewerMaxZoom, bc_chat_media_viewer_max_zoom, 800, 100, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum zoom of the chat media viewer in percent")
