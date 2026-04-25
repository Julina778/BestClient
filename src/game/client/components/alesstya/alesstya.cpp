
#include <algorithm>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <generated/protocol.h>
#include <vector>

#include "alesstya.h"

void CAlesstya::OnRender()
{
}

CAlesstya::CAlesstya()
{
	OnReset();
}

void CAlesstya::OnConsoleInit()
{
	Console()->Chain("al_skin_stealer", ConchainSkinStealer, this);
}

void CAlesstya::EnableComponent(const char *pComponent, const char *pNoteShort, const char *pNoteLong)
{
	// update
	for(auto &Component : m_aEnabledComponents)
	{
		if(str_comp(Component.m_aName, pComponent))
			continue;
		str_copy(Component.m_aName, pComponent, sizeof(Component.m_aName));
		if(pNoteShort)
			str_copy(Component.m_aNoteShort, pNoteShort, sizeof(Component.m_aNoteShort));
		if(pNoteLong)
			str_copy(Component.m_aNoteLong, pNoteLong, sizeof(Component.m_aNoteLong));
		return;
	}
	// insert
	for(auto &Component : m_aEnabledComponents)
	{
		if(Component.m_aName[0] != '\0')
			continue;
		str_copy(Component.m_aName, pComponent, sizeof(Component.m_aName));
		Component.m_aNoteShort[0] = '\0';
		Component.m_aNoteLong[0] = '\0';
		if(pNoteShort)
			str_copy(Component.m_aNoteShort, pNoteShort, sizeof(Component.m_aNoteShort));
		if(pNoteLong)
			str_copy(Component.m_aNoteLong, pNoteLong, sizeof(Component.m_aNoteLong));
		return;
	}
}

void CAlesstya::DisableComponent(const char *pComponent)
{
	// update
	for(auto &Component : m_aEnabledComponents)
	{
		if(str_comp(Component.m_aName, pComponent))
			continue;
		Component.m_aName[0] = '\0';
		return;
	}
}

bool CAlesstya::SetComponentNoteShort(const char *pComponent, const char *pNote)
{
	if(!pNote)
		return false;
	for(auto &Component : m_aEnabledComponents)
	{
		if(str_comp(Component.m_aName, pComponent))
			continue;
		str_copy(Component.m_aNoteShort, pNote, sizeof(Component.m_aNoteShort));
		return true;
	}
	return false;
}

bool CAlesstya::SetComponentNoteLong(const char *pComponent, const char *pNote)
{
	if(!pNote)
		return false;
	for(auto &Component : m_aEnabledComponents)
	{
		if(str_comp(Component.m_aName, pComponent))
			continue;
		str_copy(Component.m_aNoteLong, pNote, sizeof(Component.m_aNoteLong));
		return true;
	}
	return false;
}

void CAlesstya::OnInit()
{
	for(auto &c : m_aEnabledComponents)
	{
		c.m_aName[0] = '\0';
		c.m_aNoteShort[0] = '\0';
		c.m_aNoteLong[0] = '\0';
	}
	UpdateComponents();
}

void CAlesstya::OnShutdown()
{
	RestoreSkins();
}

void CAlesstya::UpdateComponents()
{
	if(g_Config.m_ClSkinStealer)
		EnableComponent("skin stealer");
	else
		DisableComponent("skin stealer");
}

void CAlesstya::ConchainSkinStealer(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	if(pResult->GetInteger(0) == g_Config.m_ClSkinStealer)
	{
		dbg_msg("Alesstya", "skin stealer is already %s", g_Config.m_ClSkinStealer ? "on" : "off");
		return;
	}
	CAlesstya *pSelf = (CAlesstya *)pUserData;
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 0)
		return;
	if(pResult->GetInteger(0))
	{
		pSelf->SaveSkins();
		pSelf->EnableComponent("skin stealer");
	}
	else
	{
		pSelf->RestoreSkins();
		pSelf->DisableComponent("skin stealer");
	}
}

void CAlesstya::SaveSkins()
{
	dbg_msg("Alesstya", "saved player skin '%s'", g_Config.m_ClPlayerSkin);
	str_copy(g_Config.m_ClSavedPlayerSkin, g_Config.m_ClPlayerSkin, sizeof(g_Config.m_ClSavedPlayerSkin));
	g_Config.m_ClSavedPlayerUseCustomColor = g_Config.m_ClPlayerUseCustomColor;
	g_Config.m_ClSavedPlayerColorBody = g_Config.m_ClPlayerColorBody;
	g_Config.m_ClSavedPlayerColorFeet = g_Config.m_ClPlayerColorFeet;

	dbg_msg("Alesstya", "saved dummy skin '%s'", g_Config.m_ClDummySkin);
	str_copy(g_Config.m_ClSavedDummySkin, g_Config.m_ClDummySkin, sizeof(g_Config.m_ClSavedDummySkin));
	g_Config.m_ClSavedDummyUseCustomColor = g_Config.m_ClDummyUseCustomColor;
	g_Config.m_ClSavedDummyColorBody = g_Config.m_ClDummyColorBody;
	g_Config.m_ClSavedDummyColorFeet = g_Config.m_ClDummyColorFeet;
}

void CAlesstya::RestoreSkins()
{
	bool ChangedPlayer = false;
	bool ChangedDummy = false;

	if(str_comp(g_Config.m_ClPlayerSkin, g_Config.m_ClSavedPlayerSkin) != 0)
	{
		str_copy(g_Config.m_ClPlayerSkin, g_Config.m_ClSavedPlayerSkin, sizeof(g_Config.m_ClPlayerSkin));
		ChangedPlayer = true;
	}
	if(g_Config.m_ClPlayerUseCustomColor != g_Config.m_ClSavedPlayerUseCustomColor ||
		g_Config.m_ClPlayerColorBody != g_Config.m_ClSavedPlayerColorBody ||
		g_Config.m_ClPlayerColorFeet != g_Config.m_ClSavedPlayerColorFeet)
	{
		g_Config.m_ClPlayerUseCustomColor = g_Config.m_ClSavedPlayerUseCustomColor;
		g_Config.m_ClPlayerColorBody = g_Config.m_ClSavedPlayerColorBody;
		g_Config.m_ClPlayerColorFeet = g_Config.m_ClSavedPlayerColorFeet;
		ChangedPlayer = true;
	}

	if(ChangedPlayer)
	{
		dbg_msg("Alesstya", "restored player skin '%s'", g_Config.m_ClSavedPlayerSkin);
		GameClient()->SendInfo(false);
	}

	if(str_comp(g_Config.m_ClDummySkin, g_Config.m_ClSavedDummySkin) != 0)
	{
		str_copy(g_Config.m_ClDummySkin, g_Config.m_ClSavedDummySkin, sizeof(g_Config.m_ClDummySkin));
		ChangedDummy = true;
	}
	if(g_Config.m_ClDummyUseCustomColor != g_Config.m_ClSavedDummyUseCustomColor ||
		g_Config.m_ClDummyColorBody != g_Config.m_ClSavedDummyColorBody ||
		g_Config.m_ClDummyColorFeet != g_Config.m_ClSavedDummyColorFeet)
	{
		g_Config.m_ClDummyUseCustomColor = g_Config.m_ClSavedDummyUseCustomColor;
		g_Config.m_ClDummyColorBody = g_Config.m_ClSavedDummyColorBody;
		g_Config.m_ClDummyColorFeet = g_Config.m_ClSavedDummyColorFeet;
		ChangedDummy = true;
	}

	if(ChangedDummy)
	{
		dbg_msg("Alesstya", "restored dummy skin '%s'", g_Config.m_ClSavedDummySkin);
		GameClient()->SendDummyInfo(false);
	}

	if(!ChangedPlayer && !ChangedDummy)
	{
		dbg_msg("Alesstya", "no skins restored");
	}
}
