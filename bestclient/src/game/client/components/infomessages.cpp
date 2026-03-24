/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "infomessages.h"

#include <base/color.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <engine/textrender.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/bc_ui_animations.h>
#include <game/client/components/hud_layout.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/client/prediction/gameworld.h>
#include <game/localization.h>

static constexpr float ROW_HEIGHT = 46.0f;
static constexpr float FONT_SIZE = 36.0f;
static constexpr float RACE_FLAG_SIZE = 52.0f;
static constexpr float KILLFEED_COORD_SCALE = 6.0f;

static ColorRGBA HudLayoutBackgroundColor(unsigned PackedColor)
{
	return color_cast<ColorRGBA>(ColorHSLA(PackedColor, true));
}

static int HudLayoutBackgroundCorners(CGameClient *pGameClient, int DefaultCorners, float RectX, float RectY, float RectW, float RectH, float CanvasWidth, float CanvasHeight)
{
	(void)pGameClient;
	return HudLayout::BackgroundCorners(DefaultCorners, RectX, RectY, RectW, RectH, CanvasWidth, CanvasHeight);
}

void CInfoMessages::OnWindowResize()
{
	for(auto &InfoMsg : m_aInfoMsgs)
	{
		DeleteTextContainers(InfoMsg);
	}
}

void CInfoMessages::OnReset()
{
	m_InfoMsgCurrent = 0;
	for(auto &InfoMsg : m_aInfoMsgs)
	{
		ResetMessage(InfoMsg);
	}
}

void CInfoMessages::DeleteTextContainers(CInfoMsg &InfoMsg)
{
	TextRender()->DeleteTextContainer(InfoMsg.m_VictimTextContainerIndex);
	TextRender()->DeleteTextContainer(InfoMsg.m_KillerTextContainerIndex);
	TextRender()->DeleteTextContainer(InfoMsg.m_DiffTextContainerIndex);
	TextRender()->DeleteTextContainer(InfoMsg.m_TimeTextContainerIndex);
	InfoMsg.m_VictimTextContainerIndex.Reset();
	InfoMsg.m_KillerTextContainerIndex.Reset();
	InfoMsg.m_DiffTextContainerIndex.Reset();
	InfoMsg.m_TimeTextContainerIndex.Reset();
	InfoMsg.m_FontSize = -1.0f;
}

void CInfoMessages::ResetMessage(CInfoMsg &InfoMsg)
{
	InfoMsg.m_Tick = -1;
	std::fill(std::begin(InfoMsg.m_apVictimManagedTeeRenderInfos), std::end(InfoMsg.m_apVictimManagedTeeRenderInfos), nullptr);
	InfoMsg.m_pKillerManagedTeeRenderInfo = nullptr;
	DeleteTextContainers(InfoMsg);
}

void CInfoMessages::OnInit()
{
	Graphics()->SetColor(1.f, 1.f, 1.f, 1.f);
	m_SpriteQuadContainerIndex = Graphics()->CreateQuadContainer(false);

	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	Graphics()->QuadContainerAddSprite(m_SpriteQuadContainerIndex, 0.f, 0.f, 28.f, 56.f);
	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	Graphics()->QuadContainerAddSprite(m_SpriteQuadContainerIndex, 0.f, 0.f, 28.f, 56.f);

	Graphics()->QuadsSetSubset(1, 0, 0, 1);
	Graphics()->QuadContainerAddSprite(m_SpriteQuadContainerIndex, 0.f, 0.f, 28.f, 56.f);
	Graphics()->QuadsSetSubset(1, 0, 0, 1);
	Graphics()->QuadContainerAddSprite(m_SpriteQuadContainerIndex, 0.f, 0.f, 28.f, 56.f);

	for(int i = 0; i < NUM_WEAPONS; ++i)
	{
		float ScaleX, ScaleY;
		Graphics()->QuadsSetSubset(0, 0, 1, 1);
		Graphics()->GetSpriteScale(g_pData->m_Weapons.m_aId[i].m_pSpriteBody, ScaleX, ScaleY);
		Graphics()->QuadContainerAddSprite(m_SpriteQuadContainerIndex, 96.f * ScaleX, 96.f * ScaleY);
	}

	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_QuadOffsetRaceFlag = Graphics()->QuadContainerAddSprite(m_SpriteQuadContainerIndex, 0.0f, 0.0f, RACE_FLAG_SIZE, RACE_FLAG_SIZE);

	Graphics()->QuadContainerUpload(m_SpriteQuadContainerIndex);
}

CInfoMessages::CInfoMsg CInfoMessages::CreateInfoMsg(EType Type)
{
	CInfoMsg InfoMsg;
	InfoMsg.m_Type = Type;
	InfoMsg.m_Tick = Client()->GameTick(g_Config.m_ClDummy);

	for(int i = 0; i < MAX_KILLMSG_TEAM_MEMBERS; i++)
	{
		InfoMsg.m_aVictimIds[i] = -1;
		InfoMsg.m_apVictimManagedTeeRenderInfos[i] = nullptr;
	}
	InfoMsg.m_VictimDDTeam = 0;
	InfoMsg.m_aVictimName[0] = '\0';
	InfoMsg.m_VictimTextContainerIndex.Reset();

	InfoMsg.m_KillerId = -1;
	InfoMsg.m_aKillerName[0] = '\0';
	InfoMsg.m_KillerTextContainerIndex.Reset();
	InfoMsg.m_pKillerManagedTeeRenderInfo = nullptr;

	InfoMsg.m_Weapon = -1;
	InfoMsg.m_ModeSpecial = 0;
	InfoMsg.m_FlagCarrierBlue = -1;
	InfoMsg.m_TeamSize = 0;

	InfoMsg.m_Diff = 0;
	InfoMsg.m_aTimeText[0] = '\0';
	InfoMsg.m_aDiffText[0] = '\0';
	InfoMsg.m_TimeTextContainerIndex.Reset();
	InfoMsg.m_DiffTextContainerIndex.Reset();
	InfoMsg.m_RecordPersonal = false;
	InfoMsg.m_FontSize = -1.0f;
	return InfoMsg;
}

void CInfoMessages::AddInfoMsg(const CInfoMsg &InfoMsg)
{
	dbg_assert(InfoMsg.m_TeamSize >= 0 && InfoMsg.m_TeamSize <= MAX_KILLMSG_TEAM_MEMBERS, "Info message team size invalid");
	dbg_assert(InfoMsg.m_KillerId < 0 || InfoMsg.m_pKillerManagedTeeRenderInfo != nullptr, "Info message killer invalid");
	for(int i = 0; i < InfoMsg.m_TeamSize; i++)
	{
		dbg_assert(InfoMsg.m_aVictimIds[i] >= 0 && InfoMsg.m_apVictimManagedTeeRenderInfos[i] != nullptr, "Info message victim invalid");
	}

	const float Height = 1.5f * 400.0f * 3.0f;
	const float Width = Height * Graphics()->ScreenAspect();

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->MapScreen(0, 0, Width, Height);

	m_InfoMsgCurrent = (m_InfoMsgCurrent + 1) % MAX_INFOMSGS;
	DeleteTextContainers(m_aInfoMsgs[m_InfoMsgCurrent]);
	m_aInfoMsgs[m_InfoMsgCurrent] = InfoMsg;
	CreateTextContainersIfNotCreated(m_aInfoMsgs[m_InfoMsgCurrent], FONT_SIZE);

	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

void CInfoMessages::CreateTextContainersIfNotCreated(CInfoMsg &InfoMsg, float FontSize)
{
	const auto &&NameColor = [&](int ClientId) -> ColorRGBA {
		unsigned Color;
		if(ClientId == GameClient()->m_Snap.m_LocalClientId)
		{
			Color = g_Config.m_ClKillMessageHighlightColor;
		}
		else
		{
			Color = g_Config.m_ClKillMessageNormalColor;
		}
		return color_cast<ColorRGBA>(ColorHSLA(Color));
	};

	if(absolute(InfoMsg.m_FontSize - FontSize) > 0.001f)
		DeleteTextContainers(InfoMsg);

	if(!InfoMsg.m_VictimTextContainerIndex.Valid() && InfoMsg.m_aVictimName[0] != '\0')
	{
		CTextCursor Cursor;
		Cursor.m_FontSize = FontSize;
		TextRender()->TextColor(NameColor(InfoMsg.m_aVictimIds[0]));
		TextRender()->CreateTextContainer(InfoMsg.m_VictimTextContainerIndex, &Cursor, InfoMsg.m_aVictimName);
	}

	if(!InfoMsg.m_KillerTextContainerIndex.Valid() && InfoMsg.m_aKillerName[0] != '\0')
	{
		CTextCursor Cursor;
		Cursor.m_FontSize = FontSize;
		TextRender()->TextColor(NameColor(InfoMsg.m_KillerId));
		TextRender()->CreateTextContainer(InfoMsg.m_KillerTextContainerIndex, &Cursor, InfoMsg.m_aKillerName);
	}

	if(!InfoMsg.m_DiffTextContainerIndex.Valid() && InfoMsg.m_aDiffText[0] != '\0')
	{
		CTextCursor Cursor;
		Cursor.m_FontSize = FontSize;

		if(InfoMsg.m_Diff > 0)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, 1.0f); // red
		else if(InfoMsg.m_Diff < 0)
			TextRender()->TextColor(0.5f, 1.0f, 0.5f, 1.0f); // green
		else
			TextRender()->TextColor(TextRender()->DefaultTextColor());

		TextRender()->CreateTextContainer(InfoMsg.m_DiffTextContainerIndex, &Cursor, InfoMsg.m_aDiffText);
	}

	if(!InfoMsg.m_TimeTextContainerIndex.Valid() && InfoMsg.m_aTimeText[0] != '\0')
	{
		CTextCursor Cursor;
		Cursor.m_FontSize = FontSize;
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		TextRender()->CreateTextContainer(InfoMsg.m_TimeTextContainerIndex, &Cursor, InfoMsg.m_aTimeText);
	}

	TextRender()->TextColor(TextRender()->DefaultTextColor());
	InfoMsg.m_FontSize = FontSize;
}

void CInfoMessages::OnMessage(int MsgType, void *pRawMsg)
{
	if(GameClient()->m_SuppressEvents)
		return;

	switch(MsgType)
	{
	case NETMSGTYPE_SV_KILLMSGTEAM:
		OnTeamKillMessage(static_cast<CNetMsg_Sv_KillMsgTeam *>(pRawMsg));
		break;
	case NETMSGTYPE_SV_KILLMSG:
		OnKillMessage(static_cast<CNetMsg_Sv_KillMsg *>(pRawMsg));
		break;
	case NETMSGTYPE_SV_RACEFINISH:
		OnRaceFinishMessage(static_cast<CNetMsg_Sv_RaceFinish *>(pRawMsg));
		break;
	}
}

void CInfoMessages::OnTeamKillMessage(const CNetMsg_Sv_KillMsgTeam *pMsg)
{
	std::vector<std::pair<int, int>> vStrongWeakSorted;
	vStrongWeakSorted.reserve(MAX_CLIENTS);
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		if(GameClient()->m_aClients[ClientId].m_Active &&
			GameClient()->m_Teams.Team(ClientId) == pMsg->m_Team)
		{
			CCharacter *pChr = GameClient()->m_GameWorld.GetCharacterById(ClientId);
			vStrongWeakSorted.emplace_back(ClientId, pMsg->m_First == ClientId ? MAX_CLIENTS : (pChr ? pChr->GetStrongWeakId() : 0));
		}
	}
	std::stable_sort(vStrongWeakSorted.begin(), vStrongWeakSorted.end(), [](auto &Left, auto &Right) { return Left.second > Right.second; });

	CInfoMsg Kill = CreateInfoMsg(TYPE_KILL);
	Kill.m_TeamSize = minimum<int>(vStrongWeakSorted.size(), MAX_KILLMSG_TEAM_MEMBERS);

	Kill.m_VictimDDTeam = pMsg->m_Team;
	for(int i = 0; i < Kill.m_TeamSize; i++)
	{
		const int VictimId = vStrongWeakSorted[i].first;
		Kill.m_aVictimIds[i] = VictimId;
		Kill.m_apVictimManagedTeeRenderInfos[i] = GameClient()->CreateManagedTeeRenderInfo(GameClient()->m_aClients[VictimId]);
	}
	str_format(Kill.m_aVictimName, sizeof(Kill.m_aVictimName), Localize("Team %d"), pMsg->m_Team);

	AddInfoMsg(Kill);
}

void CInfoMessages::OnKillMessage(const CNetMsg_Sv_KillMsg *pMsg)
{
	CInfoMsg Kill = CreateInfoMsg(TYPE_KILL);

	if(GameClient()->m_aClients[pMsg->m_Victim].m_Active)
	{
		Kill.m_TeamSize = 1;
		Kill.m_aVictimIds[0] = pMsg->m_Victim;
		Kill.m_VictimDDTeam = GameClient()->m_Teams.Team(Kill.m_aVictimIds[0]);
		str_copy(Kill.m_aVictimName, GameClient()->m_aClients[Kill.m_aVictimIds[0]].m_aName);
		Kill.m_apVictimManagedTeeRenderInfos[0] = GameClient()->CreateManagedTeeRenderInfo(GameClient()->m_aClients[Kill.m_aVictimIds[0]]);
	}

	if(GameClient()->m_aClients[pMsg->m_Killer].m_Active)
	{
		Kill.m_KillerId = pMsg->m_Killer;
		str_copy(Kill.m_aKillerName, GameClient()->m_aClients[Kill.m_KillerId].m_aName);
		Kill.m_pKillerManagedTeeRenderInfo = GameClient()->CreateManagedTeeRenderInfo(GameClient()->m_aClients[Kill.m_KillerId]);
	}

	Kill.m_Weapon = pMsg->m_Weapon;
	Kill.m_ModeSpecial = pMsg->m_ModeSpecial;
	Kill.m_FlagCarrierBlue = GameClient()->m_Snap.m_pGameDataObj ? GameClient()->m_Snap.m_pGameDataObj->m_FlagCarrierBlue : -1;

	if(Kill.m_TeamSize == 0 && Kill.m_KillerId == -1 && Kill.m_Weapon < 0)
	{
		return; // message would be empty
	}

	AddInfoMsg(Kill);
}

void CInfoMessages::OnRaceFinishMessage(const CNetMsg_Sv_RaceFinish *pMsg)
{
	if(!GameClient()->m_aClients[pMsg->m_ClientId].m_Active)
	{
		return;
	}

	CInfoMsg Finish = CreateInfoMsg(TYPE_FINISH);

	Finish.m_TeamSize = 1;
	Finish.m_aVictimIds[0] = pMsg->m_ClientId;
	Finish.m_VictimDDTeam = GameClient()->m_Teams.Team(Finish.m_aVictimIds[0]);
	str_copy(Finish.m_aVictimName, GameClient()->m_aClients[Finish.m_aVictimIds[0]].m_aName);
	Finish.m_apVictimManagedTeeRenderInfos[0] = GameClient()->CreateManagedTeeRenderInfo(GameClient()->m_aClients[pMsg->m_ClientId]);

	Finish.m_Diff = pMsg->m_Diff;
	Finish.m_RecordPersonal = pMsg->m_RecordPersonal || pMsg->m_RecordServer;
	if(Finish.m_Diff)
	{
		char aBuf[64];
		str_time_float(absolute(Finish.m_Diff) / 1000.0f, TIME_HOURS_CENTISECS, aBuf, sizeof(aBuf));
		str_format(Finish.m_aDiffText, sizeof(Finish.m_aDiffText), "(%c%s)", Finish.m_Diff < 0 ? '-' : '+', aBuf);
	}
	str_time_float(pMsg->m_Time / 1000.0f, TIME_HOURS_CENTISECS, Finish.m_aTimeText, sizeof(Finish.m_aTimeText));

	AddInfoMsg(Finish);
}

float CInfoMessages::KillMsgWidth(const CInfoMsg &InfoMsg, float Scale) const
{
	float Width = 0.0f;
	if(InfoMsg.m_VictimTextContainerIndex.Valid())
		Width += TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_VictimTextContainerIndex).m_W;
	Width += 24.0f * Scale;
	Width += 44.0f * Scale * InfoMsg.m_TeamSize;
	Width += 32.0f * Scale;
	Width += 52.0f * Scale;

	if(InfoMsg.m_aVictimIds[0] != InfoMsg.m_KillerId)
	{
		Width += 24.0f * Scale;
		Width += 32.0f * Scale;
		if(InfoMsg.m_KillerTextContainerIndex.Valid())
			Width += TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_KillerTextContainerIndex).m_W;
	}

	return Width;
}

float CInfoMessages::FinishMsgWidth(const CInfoMsg &InfoMsg, float Scale) const
{
	float Width = 0.0f;
	if(InfoMsg.m_DiffTextContainerIndex.Valid())
		Width += TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_DiffTextContainerIndex).m_W;
	if(InfoMsg.m_TimeTextContainerIndex.Valid())
		Width += TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_TimeTextContainerIndex).m_W;
	Width += RACE_FLAG_SIZE * Scale;
	if(InfoMsg.m_VictimTextContainerIndex.Valid())
		Width += TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_VictimTextContainerIndex).m_W;
	Width += 24.0f * Scale;
	return Width;
}

void CInfoMessages::RenderKillMsg(const CInfoMsg &InfoMsg, float x, float y, float Scale, float Alpha)
{
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);

	ColorRGBA TextColor;
	if(InfoMsg.m_VictimDDTeam)
		TextColor = GameClient()->GetDDTeamColor(InfoMsg.m_VictimDDTeam, 0.75f);
	else
		TextColor = TextRender()->DefaultTextColor();
	TextColor = TextColor.WithMultipliedAlpha(Alpha);
	const ColorRGBA TextOutlineColor = TextRender()->DefaultTextOutlineColor().WithMultipliedAlpha(Alpha);

	// render victim name
	if(InfoMsg.m_VictimTextContainerIndex.Valid())
	{
		x -= TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_VictimTextContainerIndex).m_W;
		TextRender()->RenderTextContainer(InfoMsg.m_VictimTextContainerIndex, TextColor, TextOutlineColor, x, y + (ROW_HEIGHT - FONT_SIZE) * 0.5f * Scale);
	}

	// render victim flag
	x -= 24.0f * Scale;
	if(GameClient()->m_Snap.m_pGameInfoObj && (GameClient()->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) && (InfoMsg.m_ModeSpecial & 1))
	{
		int QuadOffset;
		if(InfoMsg.m_aVictimIds[0] == InfoMsg.m_FlagCarrierBlue)
		{
			Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteFlagBlue);
			QuadOffset = 0;
		}
		else
		{
			Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteFlagRed);
			QuadOffset = 1;
		}
		Graphics()->RenderQuadContainerAsSprite(m_SpriteQuadContainerIndex, QuadOffset, x, y - 16.0f * Scale, Scale, Scale);
	}

	// render victim tees
	for(int j = (InfoMsg.m_TeamSize - 1); j >= 0; j--)
	{
		CTeeRenderInfo TeeRenderInfo = InfoMsg.m_apVictimManagedTeeRenderInfos[j]->TeeRenderInfo();
		TeeRenderInfo.m_Size *= Scale;
		vec2 OffsetToMid;
		CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &TeeRenderInfo, OffsetToMid);
		const vec2 TeeRenderPos = vec2(x, y + ROW_HEIGHT * 0.5f * Scale + OffsetToMid.y);
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeRenderInfo, EMOTE_PAIN, vec2(-1, 0), TeeRenderPos, Alpha);
		x -= 44.0f * Scale;
	}

	// render weapon
	x -= 32.0f * Scale;
	if(InfoMsg.m_Weapon >= 0)
	{
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_aSpriteWeapons[InfoMsg.m_Weapon]);
		Graphics()->RenderQuadContainerAsSprite(m_SpriteQuadContainerIndex, 4 + InfoMsg.m_Weapon, x, y + 28.0f * Scale, Scale, Scale);
	}
	x -= 52.0f * Scale;

	// render killer (only if different from victim)
	if(InfoMsg.m_aVictimIds[0] != InfoMsg.m_KillerId)
	{
		// render killer flag
		if(GameClient()->m_Snap.m_pGameInfoObj && (GameClient()->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) && (InfoMsg.m_ModeSpecial & 2))
		{
			int QuadOffset;
			if(InfoMsg.m_KillerId == InfoMsg.m_FlagCarrierBlue)
			{
				Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteFlagBlue);
				QuadOffset = 2;
			}
			else
			{
				Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteFlagRed);
				QuadOffset = 3;
			}
			Graphics()->RenderQuadContainerAsSprite(m_SpriteQuadContainerIndex, QuadOffset, x - 56.0f * Scale, y - 16.0f * Scale, Scale, Scale);
		}

		// render killer tee
		x -= 24.0f * Scale;
		if(InfoMsg.m_pKillerManagedTeeRenderInfo != nullptr)
		{
			CTeeRenderInfo TeeRenderInfo = InfoMsg.m_pKillerManagedTeeRenderInfo->TeeRenderInfo();
			TeeRenderInfo.m_Size *= Scale;
			vec2 OffsetToMid;
			CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &TeeRenderInfo, OffsetToMid);
			const vec2 TeeRenderPos = vec2(x, y + ROW_HEIGHT * 0.5f * Scale + OffsetToMid.y);
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeRenderInfo, EMOTE_ANGRY, vec2(1, 0), TeeRenderPos, Alpha);
		}
		x -= 32.0f * Scale;

		// render killer name
		if(InfoMsg.m_KillerTextContainerIndex.Valid())
		{
			x -= TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_KillerTextContainerIndex).m_W;
			TextRender()->RenderTextContainer(InfoMsg.m_KillerTextContainerIndex, TextColor, TextOutlineColor, x, y + (ROW_HEIGHT - FONT_SIZE) * 0.5f * Scale);
		}
	}

	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CInfoMessages::RenderFinishMsg(const CInfoMsg &InfoMsg, float x, float y, float Scale, float Alpha)
{
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
	const ColorRGBA DefaultTextColor = TextRender()->DefaultTextColor().WithMultipliedAlpha(Alpha);
	const ColorRGBA DefaultTextOutlineColor = TextRender()->DefaultTextOutlineColor().WithMultipliedAlpha(Alpha);

	// render time diff
	if(InfoMsg.m_DiffTextContainerIndex.Valid())
	{
		x -= TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_DiffTextContainerIndex).m_W;
		TextRender()->RenderTextContainer(InfoMsg.m_DiffTextContainerIndex, DefaultTextColor, DefaultTextOutlineColor, x, y + (ROW_HEIGHT - FONT_SIZE) * 0.5f * Scale);
	}

	// render time
	if(InfoMsg.m_TimeTextContainerIndex.Valid())
	{
		x -= TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_TimeTextContainerIndex).m_W;
		TextRender()->RenderTextContainer(InfoMsg.m_TimeTextContainerIndex, DefaultTextColor, DefaultTextOutlineColor, x, y + (ROW_HEIGHT - FONT_SIZE) * 0.5f * Scale);
	}

	// render flag
	x -= RACE_FLAG_SIZE * Scale;
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_RACEFLAG].m_Id);
	Graphics()->RenderQuadContainerAsSprite(m_SpriteQuadContainerIndex, m_QuadOffsetRaceFlag, x, y, Scale, Scale);

	// render victim name
	if(InfoMsg.m_VictimTextContainerIndex.Valid())
	{
		x -= TextRender()->GetBoundingBoxTextContainer(InfoMsg.m_VictimTextContainerIndex).m_W;
		ColorRGBA TextColor;
		if(InfoMsg.m_VictimDDTeam)
			TextColor = GameClient()->GetDDTeamColor(InfoMsg.m_VictimDDTeam, 0.75f);
		else
			TextColor = TextRender()->DefaultTextColor();
		TextRender()->RenderTextContainer(InfoMsg.m_VictimTextContainerIndex, TextColor.WithMultipliedAlpha(Alpha), DefaultTextOutlineColor, x, y + (ROW_HEIGHT - FONT_SIZE) * 0.5f * Scale);
	}

	// render victim tee
	x -= 24.0f * Scale;
	CTeeRenderInfo TeeRenderInfo = InfoMsg.m_apVictimManagedTeeRenderInfos[0]->TeeRenderInfo();
	TeeRenderInfo.m_Size *= Scale;
	vec2 OffsetToMid;
	CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &TeeRenderInfo, OffsetToMid);
	const vec2 TeeRenderPos = vec2(x, y + ROW_HEIGHT * 0.5f * Scale + OffsetToMid.y);
	const int Emote = InfoMsg.m_RecordPersonal ? EMOTE_HAPPY : EMOTE_NORMAL;
	RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeRenderInfo, Emote, vec2(-1, 0), TeeRenderPos, Alpha);

	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CInfoMessages::RenderMessages(bool ForcePreview)
{
	if(!ForcePreview && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	const float Height = 1.5f * 400.0f * 3.0f;
	const float Width = Height * Graphics()->ScreenAspect();

	const bool ShowKillMessages = g_Config.m_ClShowKillMessages != 0;
	const bool ShowFinishMessages = g_Config.m_ClShowFinishMessages != 0;
	if(!ShowKillMessages && !ShowFinishMessages)
		return;

	const auto Layout = HudLayout::Get(HudLayout::MODULE_KILLFEED, Width, Height);
	const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
	const float RowHeight = ROW_HEIGHT * Scale;
	const float FontSize = FONT_SIZE * Scale;
	const float StartX = Layout.m_X;
	int Showfps = g_Config.m_ClShowfps;
#if defined(CONF_VIDEORECORDER)
	if(IVideo::Current())
		Showfps = 0;
#endif
	const float StartY = Layout.m_Y * KILLFEED_COORD_SCALE + (Showfps ? 100.0f : 0.0f) + (g_Config.m_ClShowpred && Client()->State() != IClient::STATE_DEMOPLAYBACK ? 100.0f : 0.0f);
	const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
	const unsigned BackgroundColor = Layout.m_BackgroundColor;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->MapScreen(0, 0, Width, Height);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	int VisibleMessages = 0;
	float MaxMessageWidth = 0.0f;
	for(int i = 1; i <= MAX_INFOMSGS; i++)
	{
		CInfoMsg &InfoMsg = m_aInfoMsgs[(m_InfoMsgCurrent + i) % MAX_INFOMSGS];
		if(InfoMsg.m_Tick == -1)
		{
			continue;
		}
		if(Client()->GameTick(g_Config.m_ClDummy) > InfoMsg.m_Tick + Client()->GameTickSpeed() * 10)
		{
			ResetMessage(InfoMsg);
			continue;
		}

		const bool ShowCurrentMessage = (InfoMsg.m_Type == EType::TYPE_KILL && ShowKillMessages) || (InfoMsg.m_Type == EType::TYPE_FINISH && ShowFinishMessages);
		if(!ShowCurrentMessage)
			continue;

		CreateTextContainersIfNotCreated(InfoMsg, FontSize);
		MaxMessageWidth = maximum(MaxMessageWidth, InfoMsg.m_Type == EType::TYPE_KILL ? KillMsgWidth(InfoMsg, Scale) : FinishMsgWidth(InfoMsg, Scale));
		++VisibleMessages;
	}

	if(VisibleMessages <= 0)
	{
		Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
		return;
	}

	const float TotalHeight = VisibleMessages * RowHeight;
	const float DrawAnchorX = std::clamp(StartX, MaxMessageWidth, Width);
	const float DrawX = DrawAnchorX - MaxMessageWidth;
	const float DrawY = std::clamp(StartY, 0.0f, maximum(0.0f, Height - TotalHeight));

	if(BackgroundEnabled)
	{
		const float PaddingX = 4.0f * Scale;
		const float PaddingY = 4.0f * Scale;
		const float RectX = maximum(0.0f, DrawX - PaddingX);
		const float RectY = maximum(0.0f, DrawY - PaddingY);
		const float RectW = minimum(Width - RectX, MaxMessageWidth + PaddingX * 2.0f);
		const float RectH = minimum(Height - RectY, TotalHeight + PaddingY * 2.0f);
		const int Corners = HudLayoutBackgroundCorners(GameClient(), IGraphics::CORNER_ALL, RectX, RectY, RectW, RectH, Width, Height);
		Graphics()->DrawRect(RectX, RectY, RectW, RectH, HudLayoutBackgroundColor(BackgroundColor), Corners, 4.0f * Scale);
	}

	float y = DrawY;
	const int CurTick = Client()->GameTick(g_Config.m_ClDummy);
	const float TickSpeed = (float)Client()->GameTickSpeed();
	const float LifeTimeSeconds = 10.0f;
	for(int i = 1; i <= MAX_INFOMSGS; i++)
	{
		CInfoMsg &InfoMsg = m_aInfoMsgs[(m_InfoMsgCurrent + i) % MAX_INFOMSGS];
		if(InfoMsg.m_Tick == -1)
			continue;
		if((InfoMsg.m_Type == EType::TYPE_KILL && !ShowKillMessages) || (InfoMsg.m_Type == EType::TYPE_FINISH && !ShowFinishMessages))
			continue;

		const float MessageWidth = InfoMsg.m_Type == EType::TYPE_KILL ? KillMsgWidth(InfoMsg, Scale) : FinishMsgWidth(InfoMsg, Scale);
		const float MessageAnchorX = ForcePreview ? std::clamp(StartX, MessageWidth, Width) : DrawAnchorX;

		float Alpha = 1.0f;
		float XOff = 0.0f;
		float YOff = 0.0f;
		if(BCUiAnimations::Enabled() && TickSpeed > 0.0f)
		{
			const float AgeSeconds = (CurTick - InfoMsg.m_Tick) / TickSpeed;
			const float AppearDuration = 0.20f;
			const float AppearProgress = std::clamp(AgeSeconds / AppearDuration, 0.0f, 1.0f);
			const float AppearEase = BCUiAnimations::EaseInOutQuad(AppearProgress);

			const float TimeLeftSeconds = (InfoMsg.m_Tick + (int)(LifeTimeSeconds * TickSpeed) - CurTick) / TickSpeed;
			const float FadeOutDuration = 0.55f;
			const float FadeOutFactor = TimeLeftSeconds < FadeOutDuration ? std::clamp(TimeLeftSeconds / FadeOutDuration, 0.0f, 1.0f) : 1.0f;

			Alpha = AppearEase * FadeOutFactor;
			XOff = (1.0f - AppearEase) * 40.0f * Scale;
			YOff = -(1.0f - AppearEase) * 6.0f * Scale;
		}

		if(InfoMsg.m_Type == EType::TYPE_KILL)
			RenderKillMsg(InfoMsg, MessageAnchorX + XOff, y + YOff, Scale, Alpha);
		else
			RenderFinishMsg(InfoMsg, MessageAnchorX + XOff, y + YOff, Scale, Alpha);
		y += RowHeight;
	}

	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

void CInfoMessages::OnRender()
{
	RenderMessages(false);
}
