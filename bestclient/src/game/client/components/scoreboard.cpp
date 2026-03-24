/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "scoreboard.h"

#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/http.h>
#include <engine/textrender.h>

#include <generated/client_data.h>
#include <generated/client_data7.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/bc_ui_animations.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/statboard.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>

#include <array>

namespace
{
void RenderBestClientIcon(IGraphics *pGraphics, const CUIRect &Rect)
{
	pGraphics->TextureSet(g_pData->m_aImages[IMAGE_BCICON].m_Id);
	pGraphics->QuadsBegin();
	pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	pGraphics->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	const IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
	pGraphics->QuadsDrawTL(&Quad, 1);
	pGraphics->QuadsEnd();
}
}

CScoreboard::CScoreboard()
{
	OnReset();
}

float CScoreboard::GetPopupHeight(int ClientId, bool IsLocal) const
{
	constexpr float Margin = 5.0f;
	constexpr float FontSize = 12.0f;
	constexpr float ItemSpacing = 2.0f;
	constexpr float ActionSize = 25.0f;
	constexpr float ButtonSize = 17.5f;

	float Height = Margin * 2.0f + FontSize;
	if(!IsLocal)
	{
		Height += ItemSpacing * 2.0f + ActionSize;
		Height += (ItemSpacing * 2.0f + ButtonSize) * 5.0f;

		const int LocalId = GameClient()->m_aLocalIds[g_Config.m_ClDummy];
		const int LocalTeam = GameClient()->m_Teams.Team(LocalId);
		const int TargetTeam = GameClient()->m_Teams.Team(ClientId);
		const bool LocalInTeam = LocalTeam != TEAM_FLOCK && LocalTeam != TEAM_SUPER;
		const bool TargetInTeam = TargetTeam != TEAM_FLOCK && TargetTeam != TEAM_SUPER;
		const bool LocalIsTarget = LocalId == ClientId;

		int TeamButtonCount = 0;
		if(LocalInTeam && LocalTeam == TargetTeam)
			TeamButtonCount++;
		if(TargetInTeam && LocalTeam != TargetTeam)
			TeamButtonCount++;
		if(LocalInTeam && TargetTeam != LocalTeam)
			TeamButtonCount++;
		if(!LocalIsTarget && LocalInTeam && TargetTeam == LocalTeam)
			TeamButtonCount++;
		if(LocalInTeam && LocalTeam == TargetTeam)
			TeamButtonCount++;

		if(TeamButtonCount > 0)
			Height += ItemSpacing * 2.0f + TeamButtonCount * (ItemSpacing * 2.0f + ButtonSize);
	}

	Height += ItemSpacing * 2.0f + ButtonSize;
	return Height;
}

void CScoreboard::SetUiMousePos(vec2 Pos)
{
	const vec2 WindowSize = vec2(Graphics()->WindowWidth(), Graphics()->WindowHeight());
	const CUIRect *pScreen = Ui()->Screen();

	const vec2 UpdatedMousePos = Ui()->UpdatedMousePos();
	Pos = Pos / vec2(pScreen->w, pScreen->h) * WindowSize;
	Ui()->OnCursorMove(Pos.x - UpdatedMousePos.x, Pos.y - UpdatedMousePos.y);
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pSelf = static_cast<CScoreboard *>(pUserData);

	pSelf->GameClient()->m_Spectator.OnRelease();
	pSelf->GameClient()->m_Emoticon.OnRelease();

	pSelf->m_Active = pResult->GetInteger(0) != 0;

	if(!pSelf->IsActive() && pSelf->m_MouseUnlocked)
	{
		pSelf->Ui()->ClosePopupMenus();
		pSelf->m_MouseUnlocked = false;
		pSelf->SetUiMousePos(pSelf->m_LastMousePos.value());
		pSelf->m_LastMousePos = pSelf->Ui()->MousePos();
	}
}

void CScoreboard::ConToggleScoreboardCursor(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pSelf = static_cast<CScoreboard *>(pUserData);

	if(!pSelf->IsActive() ||
		pSelf->GameClient()->m_Menus.IsActive() ||
		pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		return;
	}

	pSelf->m_MouseUnlocked = !pSelf->m_MouseUnlocked;

	vec2 OldMousePos = pSelf->Ui()->MousePos();

	if(pSelf->m_LastMousePos == std::nullopt)
	{
		pSelf->SetUiMousePos(pSelf->Ui()->Screen()->Center());
	}
	else
	{
		pSelf->SetUiMousePos(pSelf->m_LastMousePos.value());
	}

	// save pos, so moving the mouse in esc menu doesn't change the position
	pSelf->m_LastMousePos = OldMousePos;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
	Console()->Register("toggle_scoreboard_cursor", "", CFGFLAG_CLIENT, ConToggleScoreboardCursor, this, "Toggle scoreboard cursor");
}

void CScoreboard::OnInit()
{
	m_DeadTeeTexture = Graphics()->LoadTexture("deadtee.png", IStorage::TYPE_ALL);
}

void CScoreboard::OnReset()
{
	m_Active = false;
	m_ServerRecord = -1.0f;
	m_MouseUnlocked = false;
	m_LastMousePos = std::nullopt;
	m_BcOpenPhase = 0.0f;
}

void CScoreboard::OnRelease()
{
	m_Active = false;

	if(m_MouseUnlocked)
	{
		Ui()->ClosePopupMenus();
		m_MouseUnlocked = false;
		SetUiMousePos(m_LastMousePos.value());
		m_LastMousePos = Ui()->MousePos();
	}
}

void CScoreboard::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_RECORD)
	{
		CNetMsg_Sv_Record *pMsg = static_cast<CNetMsg_Sv_Record *>(pRawMsg);
		m_ServerRecord = pMsg->m_ServerTimeBest / 100.0f;
	}
	else if(MsgType == NETMSGTYPE_SV_RECORDLEGACY)
	{
		CNetMsg_Sv_RecordLegacy *pMsg = static_cast<CNetMsg_Sv_RecordLegacy *>(pRawMsg);
		m_ServerRecord = pMsg->m_ServerTimeBest / 100.0f;
	}
}

bool CScoreboard::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!IsActive() || !m_MouseUnlocked)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	Ui()->OnCursorMove(x, y);

	return true;
}

bool CScoreboard::OnInput(const IInput::CEvent &Event)
{
	return IsActive() && m_MouseUnlocked;
}

void CScoreboard::OpenPlayerPopup(int ClientId, float X, float Y)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS || !GameClient()->m_aClients[ClientId].m_Active)
		return;

	m_ScoreboardPopupContext.m_pScoreboard = this;
	m_ScoreboardPopupContext.m_ClientId = ClientId;
	m_ScoreboardPopupContext.m_IsLocal = GameClient()->m_aLocalIds[0] == ClientId ||
					     (Client()->DummyConnected() && GameClient()->m_aLocalIds[1] == ClientId);

	Ui()->ClosePopupMenus();
	Ui()->DoPopupMenu(&m_ScoreboardPopupContext, X, Y, 110.0f,
		GetPopupHeight(m_ScoreboardPopupContext.m_ClientId, m_ScoreboardPopupContext.m_IsLocal), &m_ScoreboardPopupContext, PopupScoreboard);
}

void CScoreboard::RenderTitle(CUIRect TitleBar, int Team, const char *pTitle)
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	char aScore[128] = "";
	if(GameClient()->m_GameInfo.m_TimeScore)
	{
		if(m_ServerRecord > 0)
		{
			str_time_float(m_ServerRecord, TIME_HOURS, aScore, sizeof(aScore));
		}
	}
	else if(GameClient()->IsTeamPlay())
	{
		const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
		if(pGameDataObj)
		{
			str_format(aScore, sizeof(aScore), "%d", Team == TEAM_RED ? pGameDataObj->m_TeamscoreRed : pGameDataObj->m_TeamscoreBlue);
		}
	}
	else
	{
		if(GameClient()->m_Snap.m_SpecInfo.m_Active &&
			GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW &&
			GameClient()->m_Snap.m_apPlayerInfos[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId])
		{
			str_format(aScore, sizeof(aScore), "%d", GameClient()->m_Snap.m_apPlayerInfos[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId]->m_Score);
		}
		else if(GameClient()->m_Snap.m_pLocalInfo)
		{
			str_format(aScore, sizeof(aScore), "%d", GameClient()->m_Snap.m_pLocalInfo->m_Score);
		}
	}

	const float TitleFontSize = 20.0f;
	const float ScoreTextWidth = TextRender()->TextWidth(TitleFontSize, aScore);

	TitleBar.VMargin(10.0f, &TitleBar);
	CUIRect TitleLabel, ScoreLabel;
	if(Team == TEAM_RED)
	{
		TitleBar.VSplitRight(ScoreTextWidth, &TitleLabel, &ScoreLabel);
		TitleLabel.VSplitRight(5.0f, &TitleLabel, nullptr);
	}
	else
	{
		TitleBar.VSplitLeft(ScoreTextWidth, &ScoreLabel, &TitleLabel);
		TitleLabel.VSplitLeft(5.0f, nullptr, &TitleLabel);
	}

	{
		SLabelProperties Props;
		Props.m_MaxWidth = TitleLabel.w;
		Props.m_EllipsisAtEnd = true;
		Ui()->DoLabel(&TitleLabel, pTitle, TitleFontSize, Team == TEAM_RED ? TEXTALIGN_ML : TEXTALIGN_MR, Props);
	}

	if(aScore[0] != '\0')
	{
		Ui()->DoLabel(&ScoreLabel, aScore, TitleFontSize, Team == TEAM_RED ? TEXTALIGN_MR : TEXTALIGN_ML);
	}
}

void CScoreboard::RenderGoals(CUIRect Goals)
{
	Goals.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 7.5f);
	Goals.VMargin(5.0f, &Goals);

	const float FontSize = 10.0f;
	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	char aBuf[64];

	if(pGameInfoObj->m_ScoreLimit)
	{
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), pGameInfoObj->m_ScoreLimit);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_ML);
	}

	if(pGameInfoObj->m_TimeLimit)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), pGameInfoObj->m_TimeLimit);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_MC);
	}

	if(pGameInfoObj->m_RoundNum && pGameInfoObj->m_RoundCurrent)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Round %d/%d"), pGameInfoObj->m_RoundCurrent, pGameInfoObj->m_RoundNum);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_MR);
	}
}

void CScoreboard::RenderSpectators(CUIRect Spectators)
{
	Spectators.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 7.5f);
	Spectators.Margin(5.0f, &Spectators);

	CTextCursor Cursor;
	Cursor.SetPosition(Spectators.TopLeft());
	Cursor.m_FontSize = 11.0f;
	Cursor.m_LineWidth = Spectators.w;
	Cursor.m_MaxLines = round_truncate(Spectators.h / Cursor.m_FontSize);

	int RemainingSpectators = 0;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;
		++RemainingSpectators;
	}

	TextRender()->TextEx(&Cursor, Localize("Spectators"));

	if(RemainingSpectators > 0)
	{
		TextRender()->TextEx(&Cursor, ": ");
	}

	bool CommaNeeded = false;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		if(CommaNeeded)
		{
			TextRender()->TextEx(&Cursor, ", ");
		}

		if(Cursor.m_LineCount == Cursor.m_MaxLines && RemainingSpectators >= 2)
		{
			// This is less expensive than checking with a separate invisible
			// text cursor though we waste some space at the end of the line.
			char aRemaining[64];
			str_format(aRemaining, sizeof(aRemaining), Localize("%d others…", "Spectators"), RemainingSpectators);
			TextRender()->TextEx(&Cursor, aRemaining);
			break;
		}

		if(g_Config.m_ClShowIds)
		{
			char aClientId[16];
			GameClient()->FormaBestClientId(pInfo->m_ClientId, aClientId, EClientIdFormat::NO_INDENT);
			TextRender()->TextEx(&Cursor, aClientId);
		}

		{
			const char *pClanName = GameClient()->m_aClients[pInfo->m_ClientId].m_aClan;
			if(pClanName[0] != '\0')
			{
				if(GameClient()->m_aLocalIds[g_Config.m_ClDummy] >= 0 && str_comp(pClanName, GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_aClan) == 0)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClSameClanColor)));
				}
				else
				{
					TextRender()->TextColor(ColorRGBA(0.7f, 0.7f, 0.7f));
				}

				TextRender()->TextEx(&Cursor, pClanName);
				TextRender()->TextEx(&Cursor, " ");

				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
		}

		if(GameClient()->m_aClients[pInfo->m_ClientId].m_AuthLevel)
		{
			TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClAuthedPlayerColor)));
		}

		TextRender()->TextEx(&Cursor, GameClient()->m_aClients[pInfo->m_ClientId].m_aName);
		TextRender()->TextColor(TextRender()->DefaultTextColor());

		CommaNeeded = true;
		--RemainingSpectators;
	}
}

void CScoreboard::RenderScoreboard(CUIRect Scoreboard, int Team, int CountStart, int CountEnd, CScoreboardRenderState &State)
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
	const bool TimeScore = GameClient()->m_GameInfo.m_TimeScore;
	const int NumPlayers = CountEnd - CountStart;
	const bool LowScoreboardWidth = Scoreboard.w < 350.0f;

	bool Race7 = Client()->IsSixup() && pGameInfoObj && pGameInfoObj->m_GameFlags & protocol7::GAMEFLAG_RACE;

	const bool BcAnimEnabled = false;
	const float BcGlobalAlpha = 1.0f;

	// calculate measurements
	float LineHeight;
	float TeeSizeMod;
	float Spacing;
	float RoundRadius;
	float FontSize;
	if(NumPlayers <= 8)
	{
		LineHeight = 30.0f;
		TeeSizeMod = 0.5f;
		Spacing = 8.0f;
		RoundRadius = 5.0f;
		FontSize = 12.0f;
	}
	else if(NumPlayers <= 12)
	{
		LineHeight = 25.0f;
		TeeSizeMod = 0.45f;
		Spacing = 2.5f;
		RoundRadius = 5.0f;
		FontSize = 12.0f;
	}
	else if(NumPlayers <= 16)
	{
		LineHeight = 20.0f;
		TeeSizeMod = 0.4f;
		Spacing = 0.0f;
		RoundRadius = 2.5f;
		FontSize = 12.0f;
	}
	else if(NumPlayers <= 24)
	{
		LineHeight = 13.5f;
		TeeSizeMod = 0.3f;
		Spacing = 0.0f;
		RoundRadius = 2.5f;
		FontSize = 10.0f;
	}
	else if(NumPlayers <= 32)
	{
		LineHeight = 10.0f;
		TeeSizeMod = 0.2f;
		Spacing = 0.0f;
		RoundRadius = 2.5f;
		FontSize = 8.0f;
	}
	else if(LowScoreboardWidth)
	{
		LineHeight = 7.5f;
		TeeSizeMod = 0.125f;
		Spacing = 0.0f;
		RoundRadius = 1.0f;
		FontSize = 7.0f;
	}
	else
	{
		LineHeight = 5.0f;
		TeeSizeMod = 0.1f;
		Spacing = 0.0f;
		RoundRadius = 1.0f;
		FontSize = 5.0f;
	}

	const float ScoreOffset = Scoreboard.x + 20.0f;
	const float ScoreLength = TextRender()->TextWidth(FontSize, TimeScore ? "00:00:00" : "99999");
	const float TeeOffset = ScoreOffset + ScoreLength + 10.0f;
	const float TeeLength = 60.0f * TeeSizeMod;
	const float NameOffset = TeeOffset + TeeLength;
	const float NameLength = (LowScoreboardWidth ? 90.0f : 150.0f) - TeeLength;
	const float CountryLength = (LineHeight - Spacing - TeeSizeMod * 5.0f) * 2.0f;
	const float PingLength = 27.5f;
	const float PingOffset = Scoreboard.x + Scoreboard.w - PingLength - 10.0f;
	const float CountryOffset = PingOffset - CountryLength;
	const float ClanOffset = NameOffset + NameLength + 2.5f;
	const float ClanLength = CountryOffset - ClanOffset - 2.5f;

	// render headlines
	const float HeadlineFontsize = 11.0f;
	CUIRect Headline;
	Scoreboard.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &Scoreboard);
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	const char *pScore = TimeScore ? Localize("Time") : Localize("Score");
	TextRender()->Text(ScoreOffset + ScoreLength - TextRender()->TextWidth(HeadlineFontsize, pScore), HeadlineY, HeadlineFontsize, pScore);
	TextRender()->Text(NameOffset, HeadlineY, HeadlineFontsize, Localize("Name"));
	const char *pClanLabel = Localize("Clan");
	TextRender()->Text(ClanOffset + (ClanLength - TextRender()->TextWidth(HeadlineFontsize, pClanLabel)) / 2.0f, HeadlineY, HeadlineFontsize, pClanLabel);
	const char *pPingLabel = Localize("Ping");
	TextRender()->Text(PingOffset + PingLength - TextRender()->TextWidth(HeadlineFontsize, pPingLabel), HeadlineY, HeadlineFontsize, pPingLabel);

	struct SRenderedPlayerEntry
	{
		const CNetObj_PlayerInfo *m_pInfo;
		int m_DDTeam;
		int m_PrevDDTeam;
		int m_NextDDTeam;
		bool m_IsDead;
	};

	std::array<SRenderedPlayerEntry, MAX_CLIENTS> aRenderedPlayers{};
	int RenderedPlayerCount = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = GameClient()->m_Snap.m_apInfoByDDTeamScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		SRenderedPlayerEntry &Entry = aRenderedPlayers[RenderedPlayerCount++];
		Entry.m_pInfo = pInfo;
		Entry.m_DDTeam = GameClient()->m_Teams.Team(pInfo->m_ClientId);
		Entry.m_IsDead = Client()->m_TranslationContext.m_aClients[pInfo->m_ClientId].m_PlayerFlags7 & protocol7::PLAYERFLAG_DEAD;
	}

	for(int i = 0; i < RenderedPlayerCount; ++i)
	{
		aRenderedPlayers[i].m_PrevDDTeam = i > 0 ? aRenderedPlayers[i - 1].m_DDTeam : -1;
		aRenderedPlayers[i].m_NextDDTeam = i + 1 < RenderedPlayerCount ? aRenderedPlayers[i + 1].m_DDTeam : 0;
	}

	// render player entries
	int CountRendered = 0;
	int &CurrentDDTeamSize = State.m_CurrentDDTeamSize;

	char aBuf[64];
	int MaxTeamSize = Config()->m_SvMaxTeamSize;
	int BcAnimIndex = 0;

	for(int RenderDead = 0; RenderDead < 2; RenderDead++)
	{
		for(int i = 0; i < RenderedPlayerCount; i++)
		{
			const SRenderedPlayerEntry &RenderedPlayer = aRenderedPlayers[i];
			const CNetObj_PlayerInfo *pInfo = RenderedPlayer.m_pInfo;

			if(CountRendered++ < CountStart)
				continue;

			const int DDTeam = RenderedPlayer.m_DDTeam;
			const int PrevDDTeam = RenderedPlayer.m_PrevDDTeam;
			const int NextDDTeam = RenderedPlayer.m_NextDDTeam;
			const bool IsDead = RenderedPlayer.m_IsDead;
			if(!RenderDead && IsDead)
				continue;
			if(RenderDead && !IsDead)
				continue;

			ColorRGBA TextColor = TextRender()->DefaultTextColor();
			float BcRowEase = 1.0f;
				float BcRowAlphaMul = 1.0f;
				float BcRowYOffset = 0.0f;
				float BcRowXOffset = 0.0f;
				if(BcAnimEnabled)
				{
					const float RowPhase = BCUiAnimations::Clamp01(m_BcOpenPhase * 1.35f - (float)BcAnimIndex * 0.045f);
					BcRowEase = BCUiAnimations::EaseInOutQuad(RowPhase);
					BcRowAlphaMul = BcGlobalAlpha * BcRowEase;
					BcRowYOffset = -10.0f * (1.0f - BcRowEase);
					BcRowXOffset = -10.0f * (1.0f - BcRowEase);
				}
				TextColor.a = (RenderDead ? 0.5f : 1.0f) * BcRowAlphaMul;
				TextRender()->TextColor(TextColor);

			CUIRect RowAndSpacing, Row;
				Scoreboard.HSplitTop(LineHeight + Spacing, &RowAndSpacing, &Scoreboard);
				RowAndSpacing.HSplitTop(LineHeight, &Row, nullptr);
				RowAndSpacing.y += BcRowYOffset;
				Row.y += BcRowYOffset;
				RowAndSpacing.x += BcRowXOffset;
				Row.x += BcRowXOffset;
				++BcAnimIndex;

			// team background
			if(DDTeam != TEAM_FLOCK)
			{
				const ColorRGBA Color = GameClient()->GetDDTeamColor(DDTeam).WithAlpha(0.5f).WithMultipliedAlpha(BcRowAlphaMul);
				int TeamRectCorners = 0;
				if(PrevDDTeam != DDTeam)
				{
					TeamRectCorners |= IGraphics::CORNER_T;
					State.m_TeamStartX = Row.x;
					State.m_TeamStartY = Row.y;
				}
				if(NextDDTeam != DDTeam)
					TeamRectCorners |= IGraphics::CORNER_B;
				RowAndSpacing.Draw(Color, TeamRectCorners, RoundRadius);

				CurrentDDTeamSize++;

				if(NextDDTeam != DDTeam)
				{
					const float TeamFontSize = FontSize / 1.5f;

					if(NumPlayers > 8)
					{
						if(DDTeam == TEAM_SUPER)
							str_copy(aBuf, Localize("Super"));
						else if(CurrentDDTeamSize <= 1)
							str_format(aBuf, sizeof(aBuf), "%d", DDTeam);
						else
							str_format(aBuf, sizeof(aBuf), Localize("%d\n(%d/%d)", "Team and size"), DDTeam, CurrentDDTeamSize, MaxTeamSize);
						TextRender()->Text(State.m_TeamStartX, maximum(State.m_TeamStartY + Row.h / 2.0f - TeamFontSize, State.m_TeamStartY + 1.5f /* padding top */), TeamFontSize, aBuf);
					}
					else
					{
						if(DDTeam == TEAM_SUPER)
							str_copy(aBuf, Localize("Super"));
						else if(CurrentDDTeamSize > 1)
							str_format(aBuf, sizeof(aBuf), Localize("Team %d (%d/%d)"), DDTeam, CurrentDDTeamSize, MaxTeamSize);
						else
							str_format(aBuf, sizeof(aBuf), Localize("Team %d"), DDTeam);
						TextRender()->Text(Row.x + Row.w / 2.0f - TextRender()->TextWidth(TeamFontSize, aBuf) / 2.0f + 5.0f, Row.y + Row.h, TeamFontSize, aBuf);
					}

					CurrentDDTeamSize = 0;
				}
			}

			// background so it's easy to find the local player or the followed one in spectator mode
			if((!GameClient()->m_Snap.m_SpecInfo.m_Active && pInfo->m_Local) ||
				(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW && pInfo->m_Local) ||
				(GameClient()->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientId == GameClient()->m_Snap.m_SpecInfo.m_SpectatorId))
			{
				Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f * BcRowAlphaMul), IGraphics::CORNER_ALL, RoundRadius);
			}

			// score
			if(Race7)
			{
				if(pInfo->m_Score == -1)
				{
					aBuf[0] = '\0';
				}
				else
				{
					// 0.7 uses milliseconds and ddnets str_time wants centiseconds
					// 0.7 servers can also send the amount of precision the client should use
					// we ignore that and always show 3 digit precision
					str_time((int64_t)absolute(pInfo->m_Score / 10), TIME_MINS_CENTISECS, aBuf, sizeof(aBuf));
				}
			}
			else if(TimeScore)
			{
				if(pInfo->m_Score == -9999)
				{
					aBuf[0] = '\0';
				}
				else
				{
					str_time((int64_t)absolute(pInfo->m_Score) * 100, TIME_HOURS, aBuf, sizeof(aBuf));
				}
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%d", std::clamp(pInfo->m_Score, -999, 99999));
			}
			const float ScoreTextWidth = TextRender()->TextWidth(FontSize, aBuf);
			const float ScoreTextX = ScoreOffset + ScoreLength - ScoreTextWidth;
			if(g_Config.m_BcClientIndicatorInScoreboard && pInfo->m_ClientId >= 0 && GameClient()->m_ClientIndicator.IsPlayerBestClient(pInfo->m_ClientId))
			{
				const float IconSize = FontSize * (0.8f + 0.3f * g_Config.m_BcClientIndicatorInSoreboardSize / 100.0f);
				const float IconSpacing = 4.0f;
				CUIRect IconRect = {
					ScoreTextX - IconSize - IconSpacing,
					Row.y + (Row.h - IconSize) / 2.0f,
					IconSize,
					IconSize};
				RenderBestClientIcon(Graphics(), IconRect);
			}
			TextRender()->Text(ScoreTextX, Row.y + (Row.h - FontSize) / 2.0f, FontSize, aBuf);

			// CTF flag
			if(pGameInfoObj && (pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) &&
				pGameDataObj && (pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientId || pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientId))
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientId ? GameClient()->m_GameSkin.m_SpriteFlagBlue : GameClient()->m_GameSkin.m_SpriteFlagRed);
				Graphics()->QuadsBegin();
				Graphics()->QuadsSetSubset(1.0f, 0.0f, 0.0f, 1.0f);
				IGraphics::CQuadItem QuadItem(TeeOffset, Row.y - 2.5f - Spacing / 2.0f, Row.h / 2.0f, Row.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			const CGameClient::CClientData &ClientData = GameClient()->m_aClients[pInfo->m_ClientId];

			if(m_MouseUnlocked)
			{
				const int ButtonResult = Ui()->DoButtonLogic(&ClientData, 0, &Row, BUTTONFLAG_LEFT | BUTTONFLAG_RIGHT);
				if(ButtonResult != 0)
				{
					OpenPlayerPopup(pInfo->m_ClientId, Ui()->MouseX(), Ui()->MouseY());
				}

				if(Ui()->HotItem() == &ClientData)
				{
					Row.Draw(ColorRGBA(0.7f, 0.7f, 0.7f, 0.7f), IGraphics::CORNER_ALL, RoundRadius);
				}
			}

			// skin
			if(RenderDead)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(m_DeadTeeTexture);
				Graphics()->QuadsBegin();
				if(GameClient()->IsTeamPlay())
				{
					Graphics()->SetColor(GameClient()->m_Skins7.GetTeamColor(true, 0, GameClient()->m_aClients[pInfo->m_ClientId].m_Team, protocol7::SKINPART_BODY));
				}
				CTeeRenderInfo TeeInfo = GameClient()->m_aClients[pInfo->m_ClientId].m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				IGraphics::CQuadItem QuadItem(TeeOffset, Row.y, TeeInfo.m_Size, TeeInfo.m_Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = ClientData.m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				vec2 OffsetToMid;
				CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &TeeInfo, OffsetToMid);
				const vec2 TeeRenderPos = vec2(TeeOffset + TeeLength / 2, Row.y + Row.h / 2.0f + OffsetToMid.y);
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);
			}

				// name
			{
				CTextCursor Cursor;
				Cursor.SetPosition(vec2(NameOffset, Row.y + (Row.h - FontSize) / 2.0f));
				Cursor.m_FontSize = FontSize;
				Cursor.m_Flags |= TEXTFLAG_ELLIPSIS_AT_END;
				Cursor.m_LineWidth = NameLength;
				if(ClientData.m_AuthLevel)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClAuthedPlayerColor)));
				}
				if(g_Config.m_ClShowIds)
				{
					char aClientId[16];
					GameClient()->FormaBestClientId(pInfo->m_ClientId, aClientId, EClientIdFormat::INDENT_AUTO);
					TextRender()->TextEx(&Cursor, aClientId);
				}

				if(pInfo->m_ClientId >= 0 && (GameClient()->m_aClients[pInfo->m_ClientId].m_Foe || GameClient()->m_aClients[pInfo->m_ClientId].m_ChatIgnore))
				{
					TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
					TextRender()->TextEx(&Cursor, FontIcons::FONT_ICON_COMMENT_SLASH);
					TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
				}

				if(pInfo->m_ClientId >= 0 && g_Config.m_TcWarList && g_Config.m_TcWarListScoreboard && GameClient()->m_WarList.GetAnyWar(pInfo->m_ClientId))
					TextRender()->TextColor(GameClient()->m_WarList.GetNameplateColor(pInfo->m_ClientId));

				TextRender()->TextEx(&Cursor, ClientData.m_aName);

				// ready / watching
				if(Client()->IsSixup() && Client()->m_TranslationContext.m_aClients[pInfo->m_ClientId].m_PlayerFlags7 & protocol7::PLAYERFLAG_READY)
				{
					TextRender()->TextColor(0.1f, 1.0f, 0.1f, TextColor.a);
					TextRender()->TextEx(&Cursor, "✓");
				}
			}

			// clan
			{
				if(GameClient()->m_aLocalIds[g_Config.m_ClDummy] >= 0 && str_comp(ClientData.m_aClan, GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_aClan) == 0)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClSameClanColor)));
				}
				else
				{
					TextRender()->TextColor(TextColor);
				}

				// BestClient
				if(pInfo->m_ClientId >= 0 && g_Config.m_TcWarList && g_Config.m_TcWarListScoreboard && GameClient()->m_WarList.GetAnyWar(pInfo->m_ClientId))
					TextRender()->TextColor(GameClient()->m_WarList.GetClanColor(pInfo->m_ClientId));

				CTextCursor Cursor;
				Cursor.SetPosition(vec2(ClanOffset + (ClanLength - minimum(TextRender()->TextWidth(FontSize, ClientData.m_aClan), ClanLength)) / 2.0f, Row.y + (Row.h - FontSize) / 2.0f));
				Cursor.m_FontSize = FontSize;
				Cursor.m_Flags |= TEXTFLAG_ELLIPSIS_AT_END;
				Cursor.m_LineWidth = ClanLength;
				TextRender()->TextEx(&Cursor, ClientData.m_aClan);
			}

			// country flag
			GameClient()->m_CountryFlags.Render(ClientData.m_Country, ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f),
				CountryOffset, Row.y + (Spacing + TeeSizeMod * 5.0f) / 2.0f, CountryLength, Row.h - Spacing - TeeSizeMod * 5.0f);

			// ping
			if(g_Config.m_ClEnablePingColor)
			{
				TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA((300.0f - std::clamp(pInfo->m_Latency, 0, 300)) / 1000.0f, 1.0f, 0.5f)));
			}
			else
			{
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
			str_format(aBuf, sizeof(aBuf), "%d", std::clamp(pInfo->m_Latency, 0, 999));
			TextRender()->Text(PingOffset + PingLength - TextRender()->TextWidth(FontSize, aBuf), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aBuf);
			TextRender()->TextColor(TextRender()->DefaultTextColor());

			if(CountRendered == CountEnd)
				break;
		}
	}
}

void CScoreboard::RenderRecordingNotification(float x)
{
	char aBuf[512] = "";

	const auto &&AppendRecorderInfo = [&](int Recorder, const char *pName) {
		if(GameClient()->DemoRecorder(Recorder)->IsRecording())
		{
			char aTime[32];
			str_time((int64_t)GameClient()->DemoRecorder(Recorder)->Length() * 100, TIME_HOURS, aTime, sizeof(aTime));
			str_append(aBuf, pName);
			str_append(aBuf, " ");
			str_append(aBuf, aTime);
			str_append(aBuf, "  ");
		}
	};

	AppendRecorderInfo(RECORDER_MANUAL, Localize("Manual"));
	AppendRecorderInfo(RECORDER_RACE, Localize("Race"));
	AppendRecorderInfo(RECORDER_AUTO, Localize("Auto"));
	AppendRecorderInfo(RECORDER_REPLAYS, Localize("Replay"));

	if(aBuf[0] == '\0')
		return;

	const float FontSize = 10.0f;

	CUIRect Rect = {x, 0.0f, TextRender()->TextWidth(FontSize, aBuf) + 30.0f, 25.0f};
	Rect.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.4f), IGraphics::CORNER_B, 7.5f);
	Rect.VSplitLeft(10.0f, nullptr, &Rect);
	Rect.VSplitRight(5.0f, &Rect, nullptr);

	CUIRect Circle;
	Rect.VSplitLeft(10.0f, &Circle, &Rect);
	Circle.HMargin((Circle.h - Circle.w) / 2.0f, &Circle);
	Circle.Draw(ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_ALL, Circle.h / 2.0f);

	Rect.VSplitLeft(5.0f, nullptr, &Rect);
	Ui()->DoLabel(&Rect, aBuf, FontSize, TEXTALIGN_ML);
}

void CScoreboard::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// Check focus mode settings
	if(g_Config.m_ClFocusMode && g_Config.m_ClFocusModeHideScoreboard)
		return;

	const bool ActiveNow = IsActive();
	const float Target = ActiveNow ? 1.0f : 0.0f;
	m_BcOpenPhase = Target;
	if(m_BcOpenPhase <= 0.0f)
		return;

	const float Alpha = 1.0f;
	const float SlideY = 0.0f;
	const float Scale = 1.0f;

	const bool InteractiveUi = ActiveNow && !GameClient()->m_Menus.IsActive() && (m_MouseUnlocked || Ui()->IsPopupOpen());
	if(InteractiveUi)
	{
		Ui()->StartCheck();
		Ui()->Update();
	}

	// if the score board is active, then we should clear the motd message as well
	if(GameClient()->m_Motd.IsActive())
		GameClient()->m_Motd.Clear();

	const CUIRect Screen = *Ui()->Screen();
	Ui()->MapScreen();

	const ColorRGBA OldTextColor = TextRender()->GetTextColor();
	const ColorRGBA OldTextOutlineColor = TextRender()->GetTextOutlineColor();
	TextRender()->TextColor(OldTextColor.WithMultipliedAlpha(Alpha));
	TextRender()->TextOutlineColor(OldTextOutlineColor.WithMultipliedAlpha(Alpha));

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const bool Teams = GameClient()->IsTeamPlay();
	const auto &aTeamSize = GameClient()->m_Snap.m_aTeamSize;
	const int NumPlayers = Teams ? maximum(aTeamSize[TEAM_RED], aTeamSize[TEAM_BLUE]) : aTeamSize[TEAM_RED];

	const float ScoreboardSmallWidth = 375.0f + 10.0f;
	const float ScoreboardWidth = !Teams && NumPlayers <= 16 ? ScoreboardSmallWidth : 750.0f;
	const float TitleHeight = 30.0f;

	CUIRect Scoreboard = {(Screen.w - ScoreboardWidth) / 2.0f, 75.0f + SlideY, ScoreboardWidth, 355.0f + TitleHeight};
	if(Scale != 1.0f)
	{
		const float CenterX = Scoreboard.x + Scoreboard.w * 0.5f;
		const float CenterY = Scoreboard.y + Scoreboard.h * 0.5f;
		Scoreboard.w *= Scale;
		Scoreboard.h *= Scale;
		Scoreboard.x = CenterX - Scoreboard.w * 0.5f;
		Scoreboard.y = CenterY - Scoreboard.h * 0.5f;
	}
	CScoreboardRenderState RenderState{};

	if(Teams)
	{
		const char *pRedTeamName = GetTeamName(TEAM_RED);
		const char *pBlueTeamName = GetTeamName(TEAM_BLUE);

		// Game over title
		const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
		if((pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER) && pGameDataObj)
		{
			char aTitle[256];
			if(pGameDataObj->m_TeamscoreRed > pGameDataObj->m_TeamscoreBlue)
			{
				TextRender()->TextColor(ColorRGBA(0.975f, 0.17f, 0.17f, 1.0f));
				if(pRedTeamName == nullptr)
				{
					str_copy(aTitle, Localize("Red team wins!"));
				}
				else
				{
					str_format(aTitle, sizeof(aTitle), Localize("%s wins!"), pRedTeamName);
				}
			}
			else if(pGameDataObj->m_TeamscoreBlue > pGameDataObj->m_TeamscoreRed)
			{
				TextRender()->TextColor(ColorRGBA(0.17f, 0.46f, 0.975f, 1.0f));
				if(pBlueTeamName == nullptr)
				{
					str_copy(aTitle, Localize("Blue team wins!"));
				}
				else
				{
					str_format(aTitle, sizeof(aTitle), Localize("%s wins!"), pBlueTeamName);
				}
			}
			else
			{
				TextRender()->TextColor(ColorRGBA(0.91f, 0.78f, 0.33f, 1.0f));
				str_copy(aTitle, Localize("Draw!"));
			}

			const float TitleFontSize = 36.0f;
			CUIRect GameOverTitle = {Scoreboard.x, Scoreboard.y - TitleFontSize - 6.0f, Scoreboard.w, TitleFontSize};
			Ui()->DoLabel(&GameOverTitle, aTitle, TitleFontSize, TEXTALIGN_MC);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}

		CUIRect RedScoreboard, BlueScoreboard, RedTitle, BlueTitle;
		Scoreboard.VSplitMid(&RedScoreboard, &BlueScoreboard, 7.5f);
		RedScoreboard.HSplitTop(TitleHeight, &RedTitle, &RedScoreboard);
		BlueScoreboard.HSplitTop(TitleHeight, &BlueTitle, &BlueScoreboard);

		RedTitle.Draw(ColorRGBA(0.975f, 0.17f, 0.17f, 0.5f), IGraphics::CORNER_T, 7.5f);
		BlueTitle.Draw(ColorRGBA(0.17f, 0.46f, 0.975f, 0.5f), IGraphics::CORNER_T, 7.5f);
		RedScoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_B, 7.5f);
		BlueScoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_B, 7.5f);

		RenderTitle(RedTitle, TEAM_RED, pRedTeamName == nullptr ? Localize("Red team") : pRedTeamName);
		RenderTitle(BlueTitle, TEAM_BLUE, pBlueTeamName == nullptr ? Localize("Blue team") : pBlueTeamName);
		RenderScoreboard(RedScoreboard, TEAM_RED, 0, NumPlayers, RenderState);
		RenderScoreboard(BlueScoreboard, TEAM_BLUE, 0, NumPlayers, RenderState);
	}
	else
	{
		Scoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 7.5f);

		const char *pTitle;
		if(pGameInfoObj && (pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER))
		{
			pTitle = Localize("Game over");
		}
		else
		{
			pTitle = Client()->GetCurrentMap();
		}

		CUIRect Title;
		Scoreboard.HSplitTop(TitleHeight, &Title, &Scoreboard);
		RenderTitle(Title, TEAM_GAME, pTitle);

		if(NumPlayers <= 16)
		{
			RenderScoreboard(Scoreboard, TEAM_GAME, 0, NumPlayers, RenderState);
		}
		else if(NumPlayers <= 64)
		{
			int PlayersPerSide;
			if(NumPlayers <= 24)
				PlayersPerSide = 12;
			else if(NumPlayers <= 32)
				PlayersPerSide = 16;
			else if(NumPlayers <= 48)
				PlayersPerSide = 24;
			else
				PlayersPerSide = 32;

			CUIRect LeftScoreboard, RightScoreboard;
			Scoreboard.VSplitMid(&LeftScoreboard, &RightScoreboard);
			RenderScoreboard(LeftScoreboard, TEAM_GAME, 0, PlayersPerSide, RenderState);
			RenderScoreboard(RightScoreboard, TEAM_GAME, PlayersPerSide, 2 * PlayersPerSide, RenderState);
		}
		else
		{
			const int NumColumns = 3;
			const int PlayersPerColumn = std::ceil(128.0f / NumColumns);
			CUIRect RemainingScoreboard = Scoreboard;
			for(int i = 0; i < NumColumns; ++i)
			{
				CUIRect Column;
				RemainingScoreboard.VSplitLeft(Scoreboard.w / NumColumns, &Column, &RemainingScoreboard);
				RenderScoreboard(Column, TEAM_GAME, i * PlayersPerColumn, (i + 1) * PlayersPerColumn, RenderState);
			}
		}
	}

	CUIRect Spectators = {(Screen.w - ScoreboardSmallWidth) / 2.0f, Scoreboard.y + Scoreboard.h + 5.0f, ScoreboardSmallWidth, 100.0f};
	if(pGameInfoObj && (pGameInfoObj->m_ScoreLimit || pGameInfoObj->m_TimeLimit || (pGameInfoObj->m_RoundNum && pGameInfoObj->m_RoundCurrent)))
	{
		CUIRect Goals;
		Spectators.HSplitTop(25.0f, &Goals, &Spectators);
		Spectators.HSplitTop(5.0f, nullptr, &Spectators);
		RenderGoals(Goals);
	}
	RenderSpectators(Spectators);

	RenderRecordingNotification((Screen.w / 7) * 4 + 10);

	if(InteractiveUi)
	{
		Ui()->RenderPopupMenus();

		if(m_MouseUnlocked)
			RenderTools()->RenderCursor(Ui()->MousePos(), 24.0f);

		Ui()->FinishCheck();
		Ui()->ClearHotkeys();
	}

	TextRender()->TextColor(OldTextColor);
	TextRender()->TextOutlineColor(OldTextOutlineColor);
}

bool CScoreboard::IsActive() const
{
	// if statboard is active don't show scoreboard
	if(GameClient()->m_Statboard.IsActive())
		return false;

	if(m_Active)
		return true;

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	if(GameClient()->m_Snap.m_pLocalInfo && !GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		// we are not a spectator, check if we are dead and the game isn't paused
		if(!GameClient()->m_Snap.m_pLocalCharacter && g_Config.m_ClScoreboardOnDeath &&
			!(pGameInfoObj && pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED))
			return true;
	}

	// if the game is over
	if(pGameInfoObj && pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
		return true;

	return false;
}

const char *CScoreboard::GetTeamName(int Team) const
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	int ClanPlayers = 0;
	const char *pClanName = nullptr;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByScore)
	{
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = GameClient()->m_aClients[pInfo->m_ClientId].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(GameClient()->m_aClients[pInfo->m_ClientId].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return nullptr;
		}
	}

	if(ClanPlayers > 1 && pClanName[0] != '\0')
		return pClanName;
	else
		return nullptr;
}

CUi::EPopupMenuFunctionResult CScoreboard::PopupScoreboard(void *pContext, CUIRect View, bool Active)
{
	CScoreboardPopupContext *pPopupContext = static_cast<CScoreboardPopupContext *>(pContext);
	CScoreboard *pScoreboard = pPopupContext->m_pScoreboard;
	CUi *pUi = pPopupContext->m_pScoreboard->Ui();

	CGameClient::CClientData &Client = pScoreboard->GameClient()->m_aClients[pPopupContext->m_ClientId];

	if(!Client.m_Active)
		return CUi::POPUP_CLOSE_CURRENT;

	const float Margin = 5.0f;
	View.Margin(Margin, &View);

	CUIRect Label, Container, Action;
	const float ItemSpacing = 2.0f;
	const float FontSize = 12.0f;

	View.HSplitTop(FontSize, &Label, &View);
	pUi->DoLabel(&Label, Client.m_aName, FontSize, TEXTALIGN_ML);

	if(!pPopupContext->m_IsLocal)
	{
		const int ActionsNum = 3;
		const float ActionSize = 25.0f;
		const float ActionSpacing = (View.w - (ActionsNum * ActionSize)) / 2;
		int ActionCorners = IGraphics::CORNER_ALL;

		View.HSplitTop(ItemSpacing * 2, nullptr, &View);
		View.HSplitTop(ActionSize, &Container, &View);

		Container.VSplitLeft(ActionSize, &Action, &Container);

		ColorRGBA FriendActionColor = Client.m_Friend ? ColorRGBA(0.95f, 0.3f, 0.3f, 0.85f * pUi->ButtonColorMul(&pPopupContext->m_FriendAction)) :
								ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f * pUi->ButtonColorMul(&pPopupContext->m_FriendAction));
		const char *pFriendActionIcon = pUi->HotItem() == &pPopupContext->m_FriendAction && Client.m_Friend ? FontIcons::FONT_ICON_HEART_CRACK : FontIcons::FONT_ICON_HEART;
		if(pUi->DoButton_FontIcon(&pPopupContext->m_FriendAction, pFriendActionIcon, Client.m_Friend, &Action, BUTTONFLAG_LEFT, ActionCorners, true, FriendActionColor))
		{
			if(Client.m_Friend)
			{
				pScoreboard->GameClient()->Friends()->RemoveFriend(Client.m_aName, Client.m_aClan);
			}
			else
			{
				pScoreboard->GameClient()->Friends()->AddFriend(Client.m_aName, Client.m_aClan);
			}
		}

		pScoreboard->GameClient()->m_Tooltips.DoToolTip(&pPopupContext->m_FriendAction, &Action, Client.m_Friend ? Localize("Remove friend") : Localize("Add friend"));

		Container.VSplitLeft(ActionSpacing, nullptr, &Container);
		Container.VSplitLeft(ActionSize, &Action, &Container);

		if(pUi->DoButton_FontIcon(&pPopupContext->m_MuteAction, FontIcons::FONT_ICON_BAN, Client.m_ChatIgnore, &Action, BUTTONFLAG_LEFT, ActionCorners))
		{
			Client.m_ChatIgnore ^= 1;
		}
		pScoreboard->GameClient()->m_Tooltips.DoToolTip(&pPopupContext->m_MuteAction, &Action, Client.m_ChatIgnore ? Localize("Unmute") : Localize("Mute"));

		Container.VSplitLeft(ActionSpacing, nullptr, &Container);
		Container.VSplitLeft(ActionSize, &Action, &Container);

		const char *EmoticonActionIcon = Client.m_EmoticonIgnore ? FontIcons::FONT_ICON_COMMENT_SLASH : FontIcons::FONT_ICON_COMMENT;
		if(pUi->DoButton_FontIcon(&pPopupContext->m_EmoticonAction, EmoticonActionIcon, Client.m_EmoticonIgnore, &Action, BUTTONFLAG_LEFT, ActionCorners))
		{
			Client.m_EmoticonIgnore ^= 1;
		}
		pScoreboard->GameClient()->m_Tooltips.DoToolTip(&pPopupContext->m_EmoticonAction, &Action, Client.m_EmoticonIgnore ? Localize("Unmute emoticons") : Localize("Mute emoticons"));

	}

	const float ButtonSize = 17.5f;
	View.HSplitTop(ItemSpacing * 2, nullptr, &View);
	View.HSplitTop(ButtonSize, &Container, &View);

	bool IsSpectating = pScoreboard->GameClient()->m_Snap.m_SpecInfo.m_Active && pScoreboard->GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == pPopupContext->m_ClientId;
	ColorRGBA SpectateButtonColor = ColorRGBA(1.0f, 1.0f, 1.0f, (IsSpectating ? 0.25f : 0.5f) * pUi->ButtonColorMul(&pPopupContext->m_SpectateButton));
	if(pUi->DoButton_PopupMenu(&pPopupContext->m_SpectateButton, Localize("Spectate"), &Container, FontSize, TEXTALIGN_MC, 0.0f, false, true, SpectateButtonColor))
	{
		if(IsSpectating)
		{
			pScoreboard->GameClient()->m_Spectator.Spectate(SPEC_FREEVIEW);
			pScoreboard->Console()->ExecuteLine("say /spec");
		}
		else
		{
			if(pScoreboard->GameClient()->m_Snap.m_SpecInfo.m_Active)
			{
				pScoreboard->GameClient()->m_Spectator.Spectate(pPopupContext->m_ClientId);
			}
			else
			{
				// escape the name
				char aEscapedCommand[2 * MAX_NAME_LENGTH + 32];
				str_copy(aEscapedCommand, "say /spec \"");
				char *pDst = aEscapedCommand + str_length(aEscapedCommand);
				str_escape(&pDst, Client.m_aName, aEscapedCommand + sizeof(aEscapedCommand));
				str_append(aEscapedCommand, "\"");

				pScoreboard->Console()->ExecuteLine(aEscapedCommand);
			}
		}
	}
	if(!pPopupContext->m_IsLocal)
	{
		View.HSplitTop(ItemSpacing * 2, nullptr, &View);
		View.HSplitTop(ButtonSize, &Container, &View);

		if(pUi->DoButton_PopupMenu(&pPopupContext->m_ProfileButton, Localize("Profile"), &Container, FontSize, TEXTALIGN_MC))
		{
			CServerInfo ServerInfo;
			pScoreboard->Client()->GetServerInfo(&ServerInfo);
			const int Community = str_comp(ServerInfo.m_aCommunityId, "kog") == 0 ? 1 : (str_comp(ServerInfo.m_aCommunityId, "unique") == 0 ? 2 : 0);

			char aCommunityLink[512];
			char aEncodedName[256];
			EscapeUrl(aEncodedName, sizeof(aEncodedName), Client.m_aName);
			if(Community == 1)
				str_format(aCommunityLink, sizeof(aCommunityLink), "https://kog.tw/#p=players&player=%s", aEncodedName);
			else if(Community == 2)
				str_format(aCommunityLink, sizeof(aCommunityLink), "https://uniqueclan.net/ranks/player/%s", aEncodedName);
			else
				str_format(aCommunityLink, sizeof(aCommunityLink), "https://ddnet.org/players/%s", aEncodedName);

			pScoreboard->Client()->ViewLink(aCommunityLink);
		}

		View.HSplitTop(ItemSpacing * 2, nullptr, &View);
		View.HSplitTop(ButtonSize, &Container, &View);

		if(pUi->DoButton_PopupMenu(&pPopupContext->m_WhisperButton, Localize("Whisper"), &Container, FontSize, TEXTALIGN_MC))
		{
			char aWhisperBuf[512];
			str_format(aWhisperBuf, sizeof(aWhisperBuf), "chat all /whisper %s ", Client.m_aName);
			pScoreboard->Console()->ExecuteLine(aWhisperBuf);
		}

		View.HSplitTop(ItemSpacing * 2, nullptr, &View);
		View.HSplitTop(ButtonSize, &Container, &View);

		if(pUi->DoButton_PopupMenu(&pPopupContext->m_VoteKickButton, Localize("Vote Kick"), &Container, FontSize, TEXTALIGN_MC))
		{
			pScoreboard->GameClient()->m_Voting.CallvoteKick(Client.ClientId(), "");
		}

		View.HSplitTop(ItemSpacing * 2, nullptr, &View);
		View.HSplitTop(ButtonSize, &Container, &View);

		if(pUi->DoButton_PopupMenu(&pPopupContext->m_ClipNameButton, Localize("Clip Name"), &Container, FontSize, TEXTALIGN_MC))
		{
			pScoreboard->Input()->SetClipboardText(Client.m_aName);
		}

		View.HSplitTop(ItemSpacing * 2, nullptr, &View);
		View.HSplitTop(ButtonSize, &Container, &View);

		if(pUi->DoButton_PopupMenu(&pPopupContext->m_SwapButton, Localize("Swap"), &Container, FontSize, TEXTALIGN_MC))
		{
			char aSwapBuf[256];
			str_format(aSwapBuf, sizeof(aSwapBuf), "say /swap %s", Client.m_aName);
			pScoreboard->Console()->ExecuteLine(aSwapBuf);
		}

		const int LocalId = pScoreboard->GameClient()->m_aLocalIds[g_Config.m_ClDummy];
		const int LocalTeam = pScoreboard->GameClient()->m_Teams.Team(LocalId);
		const int TargetTeam = pScoreboard->GameClient()->m_Teams.Team(pPopupContext->m_ClientId);
		const bool LocalInTeam = LocalTeam != TEAM_FLOCK && LocalTeam != TEAM_SUPER;
		const bool TargetInTeam = TargetTeam != TEAM_FLOCK && TargetTeam != TEAM_SUPER;
		const bool LocalIsTarget = LocalId == pPopupContext->m_ClientId;

		if(LocalInTeam || TargetInTeam)
		{
			View.HSplitTop(ItemSpacing * 2, nullptr, &View);

			bool AddedTeamButton = false;
			auto AddTeamButton = [&](CButtonContainer *pButton, const char *pLabel, auto &&OnClick) {
				if(AddedTeamButton)
					View.HSplitTop(ItemSpacing * 2, nullptr, &View);
				AddedTeamButton = true;
				View.HSplitTop(ButtonSize, &Container, &View);
				if(pUi->DoButton_PopupMenu(pButton, pLabel, &Container, FontSize, TEXTALIGN_MC))
					OnClick();
			};

			if(LocalInTeam && LocalTeam == TargetTeam)
			{
				AddTeamButton(&pPopupContext->m_TeamExitButton, Localize("Exit"), [&]() { pScoreboard->Console()->ExecuteLine("say /team 0"); });
			}
			if(TargetInTeam && LocalTeam != TargetTeam)
			{
				AddTeamButton(&pPopupContext->m_TeamJoinButton, Localize("Join"), [&]() {
					char aCmdBuf[128];
					str_format(aCmdBuf, sizeof(aCmdBuf), "say /team %d", TargetTeam);
					pScoreboard->Console()->ExecuteLine(aCmdBuf);
				});
			}
			if(LocalInTeam && TargetTeam != LocalTeam)
			{
				AddTeamButton(&pPopupContext->m_TeamInviteButton, Localize("Invite"), [&]() {
					char aCmdBuf[128];
					str_format(aCmdBuf, sizeof(aCmdBuf), "say /invite %s", Client.m_aName);
					pScoreboard->Console()->ExecuteLine(aCmdBuf);
				});
			}
			if(!LocalIsTarget && LocalInTeam && TargetTeam == LocalTeam)
			{
				AddTeamButton(&pPopupContext->m_TeamKickButton, Localize("Kick"), [&]() {
					pScoreboard->GameClient()->m_Voting.CallvoteKick(pPopupContext->m_ClientId, "");
				});
			}
			if(LocalInTeam && LocalTeam == TargetTeam)
			{
				AddTeamButton(&pPopupContext->m_TeamLockButton, Localize("Lock"), [&]() { pScoreboard->Console()->ExecuteLine("say /lock"); });
			}
		}
	}

	return CUi::POPUP_KEEP_OPEN;
}
