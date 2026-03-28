#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_BESTCLIENT_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_BESTCLIENT_H

#include <engine/shared/console.h>

#include <game/client/component.h>

class CBestClient : public CComponent
{
	static void ConToggle45Degrees(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleSmallSens(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleDeepfly(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleCinematicCamera(IConsole::IResult *pResult, void *pUserData);

	int m_45degreestoggle = 0;
	int m_45degreestogglelastinput = 0;
	int m_45degreesEnabled = 0;
	int m_Smallsenstoggle = 0;
	int m_Smallsenstogglelastinput = 0;
	int m_SmallsensEnabled = 0;
	char m_Oldmouse1Bind[128] = {};

public:
	enum EBestClientComponent
	{
		COMPONENT_VISUALS_MUSIC_PLAYER = 0,
		COMPONENT_VISUALS_CAVA,
		COMPONENT_VISUALS_CRYSTAL_LASER,
		COMPONENT_VISUALS_MEDIA_BACKGROUND,
		COMPONENT_VISUALS_MAGIC_PARTICLES,
		COMPONENT_VISUALS_ORBIT_AURA,
		COMPONENT_VISUALS_OPTIMIZER,
		COMPONENT_VISUALS_ANIMATIONS,
		COMPONENT_VISUALS_CAMERA_DRIFT,
		COMPONENT_VISUALS_DYNAMIC_FOV,
		COMPONENT_VISUALS_AFTERIMAGE,
		COMPONENT_VISUALS_FOCUS_MODE,
		COMPONENT_VISUALS_CHAT_BUBBLES,
		COMPONENT_VISUALS_3D_PARTICLES,
		COMPONENT_VISUALS_ASPECT_RATIO,
		COMPONENT_GAMEPLAY_INPUT,
		COMPONENT_GAMEPLAY_FAST_ACTIONS,
		COMPONENT_GAMEPLAY_SPEEDRUN_TIMER,
		COMPONENT_GAMEPLAY_AUTO_TEAM_LOCK,
		COMPONENT_GAMEPLAY_GORES_MODE,
		COMPONENT_OTHERS_CLIENT_INDICATOR,
		COMPONENT_TCLIENT_SETTINGS_TAB,
		COMPONENT_TCLIENT_BIND_WHEEL_TAB,
		COMPONENT_TCLIENT_WAR_LIST_TAB,
		COMPONENT_TCLIENT_CHAT_BINDS_TAB,
		COMPONENT_TCLIENT_STATUS_BAR_TAB,
		COMPONENT_TCLIENT_INFO_TAB,
		COMPONENT_TCLIENT_PROFILES_PAGE,
		COMPONENT_TCLIENT_CONFIGS_PAGE,
		NUM_COMPONENTS_EDITOR_COMPONENTS,
	};

	CBestClient();
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	bool IsComponentDisabled(EBestClientComponent Component) const;
	static bool IsComponentDisabledByMask(int Component, int MaskLo, int MaskHi);
};

#endif
