#include "rainbow.h"

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>

template<typename T>
static T color_lerp(T a, T b, float c)
{
	T result;
	for(size_t i = 0; i < 4; ++i)
		result[i] = a[i] + c * (b[i] - a[i]);
	return result;
}

bool CRainbow::ShouldUpdateColors(bool &AffectLocal, bool &AffectOthers) const
{
	AffectLocal = g_Config.m_TcRainbowTees != 0;
	AffectOthers = g_Config.m_TcRainbowTees != 0 && g_Config.m_TcRainbowOthers != 0;
	if(!AffectLocal && !g_Config.m_TcRainbowWeapon && !g_Config.m_TcRainbowHook)
		return false;
	if(g_Config.m_TcRainbowMode == 0)
		return false;
	return true;
}

void CRainbow::OnRender()
{
	if(GameClient()->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_GORES_RAINBOW))
		return;

	bool AffectLocal = false;
	bool AffectOthers = false;
	if(!ShouldUpdateColors(AffectLocal, AffectOthers))
		return;

	m_Time += Client()->RenderFrameTime() * ((float)g_Config.m_TcRainbowSpeed / 100.0f);
	float DefTick = std::fmod(m_Time, 1.0f);
	ColorRGBA Col;

	switch(g_Config.m_TcRainbowMode)
	{
	case COLORMODE_RAINBOW:
		Col = color_cast<ColorRGBA>(ColorHSLA(DefTick, 1.0f, 0.5f));
		break;
	case COLORMODE_PULSE:
		Col = color_cast<ColorRGBA>(ColorHSLA(std::fmod(std::floor(m_Time) * 0.1f, 1.0f), 1.0f, 0.5f + std::fabs(DefTick - 0.5f)));
		break;
	case COLORMODE_DARKNESS:
		Col = ColorRGBA(0.0f, 0.0f, 0.0f);
		break;
	case COLORMODE_RANDOM:
		static ColorHSLA s_Col1 = ColorHSLA(0.0f, 0.0f, 0.0f, 0.0f), s_Col2 = ColorHSLA(0.0f, 0.0f, 0.0f, 0.0f);
		if(s_Col2.a == 0.0f) // Create first target
			s_Col2 = ColorHSLA((float)rand() / (float)RAND_MAX, 1.0f, (float)rand() / (float)RAND_MAX, 1.0f);
		static float s_LastSwap = -INFINITY;
		if(m_Time - s_LastSwap > 1.0f) // Shift target to source, create new target
		{
			s_LastSwap = m_Time;
			s_Col1 = s_Col2;
			s_Col2 = ColorHSLA((float)rand() / (float)RAND_MAX, 1.0f, (float)rand() / (float)RAND_MAX, 1.0f);
		}
		Col = color_cast<ColorRGBA>(color_lerp(s_Col1, s_Col2, DefTick));
		break;
	}

	m_RainbowColor = Col;

	if(AffectLocal)
	{
		const int LocalClientId = GameClient()->m_Snap.m_LocalClientId;
		if(LocalClientId >= 0 && LocalClientId < MAX_CLIENTS && GameClient()->m_Snap.m_aCharacters[LocalClientId].m_Active)
		{
			CTeeRenderInfo *pRenderInfo = &GameClient()->m_aClients[LocalClientId].m_RenderInfo;
			pRenderInfo->m_BloodColor = Col;
			pRenderInfo->m_ColorBody = Col;
			pRenderInfo->m_ColorFeet = Col;
			pRenderInfo->m_CustomColoredSkin = true;
		}
	}

	if(!AffectOthers)
		return;

	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		if(ClientId == GameClient()->m_Snap.m_LocalClientId || !GameClient()->m_Snap.m_aCharacters[ClientId].m_Active)
			continue;

		CTeeRenderInfo *pRenderInfo = &GameClient()->m_aClients[ClientId].m_RenderInfo;
		if(!pRenderInfo->Valid())
			continue;

		pRenderInfo->m_BloodColor = Col;
		pRenderInfo->m_ColorBody = Col;
		pRenderInfo->m_ColorFeet = Col;
		pRenderInfo->m_CustomColoredSkin = true;
	}
}
