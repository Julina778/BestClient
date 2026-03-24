/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "hud.h"

#include "binds.h"
#include "camera.h"
#include "controls.h"
#include "speedrun.h"
#include "voting.h"

#include <base/color.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/bc_ui_animations.h>
#include <game/client/components/hud_layout.h>
#include <game/client/components/scoreboard.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/layers.h>
#include <game/localization.h>

#include <cmath>

CHud::CHud() : m_SpeedrunTimerExpiredTick(0)
{
	m_FPSTextContainerIndex.Reset();
	m_DDRaceEffectsTextContainerIndex.Reset();
	m_PlayerAngleTextContainerIndex.Reset();
	m_PlayerPrevAngle = -INFINITY;

	for(int i = 0; i < 2; i++)
	{
		m_aPlayerSpeedTextContainers[i].Reset();
		m_aPlayerPrevSpeed[i] = -INFINITY;
		m_aPlayerPositionContainers[i].Reset();
		m_aPlayerPrevPosition[i] = -INFINITY;
	}
}

static ColorRGBA HudModuleBackgroundColor(unsigned PackedColor)
{
	return color_cast<ColorRGBA>(ColorHSLA(PackedColor, true));
}

static int HudModuleBackgroundCorners(int DefaultCorners, float RectX, float RectY, float RectW, float RectH, float CanvasWidth, float CanvasHeight)
{
	return HudLayout::BackgroundCorners(DefaultCorners, RectX, RectY, RectW, RectH, CanvasWidth, CanvasHeight);
}

float CHud::HudAlpha() const
{
	return 1.0f;
}

ColorRGBA CHud::ApplyHudAlpha(ColorRGBA Color) const
{
	Color.a *= HudAlpha();
	return Color;
}

void CHud::SetHudColor(float r, float g, float b, float a)
{
	Graphics()->SetColor(r, g, b, a * HudAlpha());
}

void CHud::SetHudTextColor(float r, float g, float b, float a)
{
	TextRender()->TextColor(r, g, b, a * HudAlpha());
}

void CHud::SetHudTextColor(ColorRGBA Color)
{
	Color.a *= HudAlpha();
	TextRender()->TextColor(Color);
}

void CHud::ResetHudContainers()
{
	for(auto &ScoreInfo : m_aScoreInfo)
	{
		TextRender()->DeleteTextContainer(ScoreInfo.m_OptionalNameTextContainerIndex);
		TextRender()->DeleteTextContainer(ScoreInfo.m_TextRankContainerIndex);
		TextRender()->DeleteTextContainer(ScoreInfo.m_TextScoreContainerIndex);
		Graphics()->DeleteQuadContainer(ScoreInfo.m_RoundRectQuadContainerIndex);

		ScoreInfo.Reset();
	}

	TextRender()->DeleteTextContainer(m_FPSTextContainerIndex);
	TextRender()->DeleteTextContainer(m_DDRaceEffectsTextContainerIndex);
	TextRender()->DeleteTextContainer(m_PlayerAngleTextContainerIndex);
	m_PlayerPrevAngle = -INFINITY;
	for(int i = 0; i < 2; i++)
	{
		TextRender()->DeleteTextContainer(m_aPlayerSpeedTextContainers[i]);
		m_aPlayerPrevSpeed[i] = -INFINITY;
		TextRender()->DeleteTextContainer(m_aPlayerPositionContainers[i]);
		m_aPlayerPrevPosition[i] = -INFINITY;
	}
}

void CHud::OnWindowResize()
{
	ResetHudContainers();
}

void CHud::OnReset()
{
	m_TimeCpDiff = 0.0f;
	m_DDRaceTime = 0;
	m_FinishTimeLastReceivedTick = 0;
	m_TimeCpLastReceivedTick = 0;
	m_ShowFinishTime = false;
	m_ServerRecord = -1.0f;
	m_aPlayerRecord[0] = -1.0f;
	m_aPlayerRecord[1] = -1.0f;
	m_aPlayerSpeed[0] = 0;
	m_aPlayerSpeed[1] = 0;
	m_aLastPlayerSpeedChange[0] = ESpeedChange::NONE;
	m_aLastPlayerSpeedChange[1] = ESpeedChange::NONE;
	m_LastSpectatorCountTick = 0;
	m_SpeedrunTimerExpiredTick = 0;

	ResetHudContainers();
}

void CHud::OnInit()
{
	OnReset();

	SetHudColor(1.0, 1.0, 1.0, 1.0);

	m_HudQuadContainerIndex = Graphics()->CreateQuadContainer(false);
	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	PrepareAmmoHealthAndArmorQuads();

	// all cursors for the different weapons
	for(int i = 0; i < NUM_WEAPONS; ++i)
	{
		float ScaleX, ScaleY;
		Graphics()->GetSpriteScale(g_pData->m_Weapons.m_aId[i].m_pSpriteCursor, ScaleX, ScaleY);
		m_aCursorOffset[i] = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 64.f * ScaleX, 64.f * ScaleY);
	}

	// the flags
	m_FlagOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 8.f, 16.f);

	PreparePlayerStateQuads();

	Graphics()->QuadContainerUpload(m_HudQuadContainerIndex);
}

void CHud::RenderGameTimer(bool ForcePreview)
{
	const bool MusicPlayerOccupiesTimerSlot = !GameClient()->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_VISUALS_MUSIC_PLAYER) && g_Config.m_BcMusicPlayer != 0 && !(g_Config.m_ClFocusMode && g_Config.m_ClFocusModeHideSongPlayer);
	if(!ForcePreview && MusicPlayerOccupiesTimerSlot)
		return;

	const auto Layout = HudLayout::Get(HudLayout::MODULE_GAME_TIMER, m_Width, m_Height);
	const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
	const float FontSize = 10.0f * Scale;
	const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
	const unsigned BackgroundColor = Layout.m_BackgroundColor;
	const bool HasGameInfo = GameClient()->m_Snap.m_pGameInfoObj != nullptr;
	if(!HasGameInfo && !ForcePreview)
		return;

	if(ForcePreview || !HasGameInfo || !(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_SUDDENDEATH))
	{
		char aBuf[32];
		int Time = 0;
		bool WarningTime = false;
		if(HasGameInfo && GameClient()->m_Snap.m_pGameInfoObj->m_TimeLimit && (GameClient()->m_Snap.m_pGameInfoObj->m_WarmupTimer <= 0))
		{
			Time = GameClient()->m_Snap.m_pGameInfoObj->m_TimeLimit * 60 - ((Client()->GameTick(g_Config.m_ClDummy) - GameClient()->m_Snap.m_pGameInfoObj->m_RoundStartTick) / Client()->GameTickSpeed());

			if(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
				Time = 0;
			WarningTime = Time <= 60;
		}
		else if(HasGameInfo && (GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_RACETIME))
		{
			// The Warmup timer is negative in this case to make sure that incompatible clients will not see a warmup timer
			Time = (Client()->GameTick(g_Config.m_ClDummy) + GameClient()->m_Snap.m_pGameInfoObj->m_WarmupTimer) / Client()->GameTickSpeed();
		}
		else if(HasGameInfo)
			Time = (Client()->GameTick(g_Config.m_ClDummy) - GameClient()->m_Snap.m_pGameInfoObj->m_RoundStartTick) / Client()->GameTickSpeed();
		else
			Time = 12 * 60 + 34;

		str_time((int64_t)Time * 100, TIME_DAYS, aBuf, sizeof(aBuf));
		static float s_FontSize = -1.0f;
		static float s_TextWidthM = 0.0f;
		static float s_TextWidthH = 0.0f;
		static float s_TextWidth0D = 0.0f;
		static float s_TextWidth00D = 0.0f;
		static float s_TextWidth000D = 0.0f;
		if(s_FontSize != FontSize)
		{
			s_FontSize = FontSize;
			s_TextWidthM = TextRender()->TextWidth(FontSize, "00:00", -1, -1.0f);
			s_TextWidthH = TextRender()->TextWidth(FontSize, "00:00:00", -1, -1.0f);
			s_TextWidth0D = TextRender()->TextWidth(FontSize, "0d 00:00:00", -1, -1.0f);
			s_TextWidth00D = TextRender()->TextWidth(FontSize, "00d 00:00:00", -1, -1.0f);
			s_TextWidth000D = TextRender()->TextWidth(FontSize, "000d 00:00:00", -1, -1.0f);
		}
		float w = Time >= 3600 * 24 * 100 ? s_TextWidth000D : (Time >= 3600 * 24 * 10 ? s_TextWidth00D : (Time >= 3600 * 24 ? s_TextWidth0D : (Time >= 3600 ? s_TextWidthH : s_TextWidthM)));
		float DrawX = std::clamp(m_Width / 2.0f - w / 2.0f, 0.0f, maximum(0.0f, m_Width - w));
		float DrawY = std::clamp(2.0f, 0.0f, maximum(0.0f, m_Height - FontSize));
		const CUIRect PushRect = {DrawX - 2.0f * Scale, DrawY - 1.0f * Scale, w + 4.0f * Scale, FontSize + 2.0f * Scale};
		const float PushDownOffset = GameClient()->m_MusicPlayer.GetHudPushDownOffsetForRect(PushRect, m_Height, 4.0f * Scale);
		DrawY = std::clamp(DrawY + maximum(0.0f, PushDownOffset), 0.0f, maximum(0.0f, m_Height - FontSize));
		if(BackgroundEnabled)
		{
			const float RectX = DrawX - 2.0f * Scale;
			const float RectY = DrawY - 1.0f * Scale;
			const float RectW = w + 4.0f * Scale;
			const float RectH = FontSize + 2.0f * Scale;
			const int Corners = HudModuleBackgroundCorners(IGraphics::CORNER_ALL, RectX, RectY, RectW, RectH, m_Width, m_Height);
			Graphics()->DrawRect(RectX, RectY, RectW, RectH, ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)), Corners, 2.0f * Scale);
		}
		// last 60 sec red, last 10 sec blink
		if(HasGameInfo && WarningTime && (GameClient()->m_Snap.m_pGameInfoObj->m_WarmupTimer <= 0))
		{
			float Alpha = Time <= 10 && (2 * time() / time_freq()) % 2 ? 0.5f : 1.0f;
			SetHudTextColor(1.0f, 0.25f, 0.25f, Alpha);
		}
		TextRender()->Text(DrawX, DrawY, FontSize, aBuf, -1.0f);
		SetHudTextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void CHud::RenderPracticeModeBanner()
{
	const bool Visible = GameClient()->m_FastPractice.Enabled();
	static float s_Phase = 0.0f;
	const float Dt = Client()->RenderFrameTime();
	if(BCUiAnimations::Enabled())
		BCUiAnimations::UpdatePhase(s_Phase, Visible ? 1.0f : 0.0f, Dt, 0.22f);
	else
		s_Phase = Visible ? 1.0f : 0.0f;
	if(s_Phase <= 0.0f)
		return;

	const float Ease = BCUiAnimations::EaseInOutQuad(s_Phase);
	const float Alpha = Ease;
	const float YOff = -10.0f * (1.0f - Ease);

	const char *pText = Localize("Practice mode");
	const char *pSubText = Localize("you can use practice commands (/tc /invincible)");
	const float FontSize = 10.0f;
	const float SubFontSize = 8.0f;
	const float BannerTitleY = 22.0f;
	const float BannerSubTitleY = 38.0f;
	const float TextWidth = TextRender()->TextWidth(FontSize, pText, -1, -1.0f);
	const float SubTextWidth = TextRender()->TextWidth(SubFontSize, pSubText, -1, -1.0f);
	SetHudTextColor(1.0f, 1.0f, 1.0f, Alpha);
	TextRender()->Text(m_Width / 2.0f - TextWidth / 2.0f, BannerTitleY + YOff, FontSize, pText, -1.0f);
	TextRender()->Text(m_Width / 2.0f - SubTextWidth / 2.0f, BannerSubTitleY + YOff, SubFontSize, pSubText, -1.0f);
	SetHudTextColor(TextRender()->DefaultTextColor());
}

void CHud::RenderPauseNotification()
{
	const bool Visible = (GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED) != 0 &&
		(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER) == 0;
	static float s_Phase = 0.0f;
	const float Dt = Client()->RenderFrameTime();
	if(BCUiAnimations::Enabled())
		BCUiAnimations::UpdatePhase(s_Phase, Visible ? 1.0f : 0.0f, Dt, 0.22f);
	else
		s_Phase = Visible ? 1.0f : 0.0f;
	if(s_Phase <= 0.0f)
		return;

	const float Ease = BCUiAnimations::EaseInOutQuad(s_Phase);
	const float Alpha = Ease;
	const float YOff = -10.0f * (1.0f - Ease);

	const char *pText = Localize("Game paused");
	float FontSize = 20.0f;
	float w = TextRender()->TextWidth(FontSize, pText, -1, -1.0f);
	SetHudTextColor(1.0f, 1.0f, 1.0f, Alpha);
	TextRender()->Text(150.0f * Graphics()->ScreenAspect() + -w / 2.0f, 50.0f + YOff, FontSize, pText, -1.0f);
	SetHudTextColor(TextRender()->DefaultTextColor());
}

void CHud::RenderSuddenDeath()
{
	const bool Visible = (GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_SUDDENDEATH) != 0;
	static float s_Phase = 0.0f;
	const float Dt = Client()->RenderFrameTime();
	if(BCUiAnimations::Enabled())
		BCUiAnimations::UpdatePhase(s_Phase, Visible ? 1.0f : 0.0f, Dt, 0.22f);
	else
		s_Phase = Visible ? 1.0f : 0.0f;
	if(s_Phase <= 0.0f)
		return;

	const float Ease = BCUiAnimations::EaseInOutQuad(s_Phase);
	const float Alpha = Ease;
	const float YOff = -8.0f * (1.0f - Ease);

	float Half = m_Width / 2.0f;
	const char *pText = Localize("Sudden Death");
	float FontSize = 12.0f;
	float w = TextRender()->TextWidth(FontSize, pText, -1, -1.0f);
	SetHudTextColor(1.0f, 1.0f, 1.0f, Alpha);
	TextRender()->Text(Half - w / 2, 2 + YOff, FontSize, pText, -1.0f);
	SetHudTextColor(TextRender()->DefaultTextColor());
}

void CHud::RenderScoreHud(bool ForcePreview)
{
	(void)ForcePreview;
	if(!GameClient()->m_Snap.m_pGameInfoObj)
		return;

	// Match def-ddnet score HUD layout and geometry.
	if(!(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER))
	{
		float StartY = 229.0f;
		const float ScoreSingleBoxHeight = 18.0f;

		bool ForceScoreInfoInit = !m_aScoreInfo[0].m_Initialized || !m_aScoreInfo[1].m_Initialized;
		m_aScoreInfo[0].m_Initialized = m_aScoreInfo[1].m_Initialized = true;

		if(GameClient()->IsTeamPlay() && GameClient()->m_Snap.m_pGameDataObj)
		{
			char aScoreTeam[2][16];
			str_format(aScoreTeam[TEAM_RED], sizeof(aScoreTeam[TEAM_RED]), "%d", GameClient()->m_Snap.m_pGameDataObj->m_TeamscoreRed);
			str_format(aScoreTeam[TEAM_BLUE], sizeof(aScoreTeam[TEAM_BLUE]), "%d", GameClient()->m_Snap.m_pGameDataObj->m_TeamscoreBlue);

			bool aRecreateTeamScore[2] = {str_comp(aScoreTeam[0], m_aScoreInfo[0].m_aScoreText) != 0, str_comp(aScoreTeam[1], m_aScoreInfo[1].m_aScoreText) != 0};
			const int aFlagCarrier[2] = {
				GameClient()->m_Snap.m_pGameDataObj->m_FlagCarrierRed,
				GameClient()->m_Snap.m_pGameDataObj->m_FlagCarrierBlue};

			bool RecreateRect = ForceScoreInfoInit;
			for(int t = 0; t < 2; t++)
			{
				if(aRecreateTeamScore[t])
				{
					m_aScoreInfo[t].m_ScoreTextWidth = TextRender()->TextWidth(14.0f, aScoreTeam[t == 0 ? TEAM_RED : TEAM_BLUE], -1, -1.0f);
					str_copy(m_aScoreInfo[t].m_aScoreText, aScoreTeam[t == 0 ? TEAM_RED : TEAM_BLUE]);
					RecreateRect = true;
				}
			}

			static float s_TextWidth100 = TextRender()->TextWidth(14.0f, "100", -1, -1.0f);
			float ScoreWidthMax = maximum(maximum(m_aScoreInfo[0].m_ScoreTextWidth, m_aScoreInfo[1].m_ScoreTextWidth), s_TextWidth100);
			float Split = 3.0f;
			float ImageSize = (GameClient()->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) ? 16.0f : Split;
			for(int t = 0; t < 2; t++)
			{
				if(RecreateRect)
				{
					Graphics()->DeleteQuadContainer(m_aScoreInfo[t].m_RoundRectQuadContainerIndex);

					if(t == 0)
						SetHudColor(0.975f, 0.17f, 0.17f, 0.3f);
					else
						SetHudColor(0.17f, 0.46f, 0.975f, 0.3f);
					m_aScoreInfo[t].m_RoundRectQuadContainerIndex = Graphics()->CreateRectQuadContainer(m_Width - ScoreWidthMax - ImageSize - 2 * Split, StartY + t * 20, ScoreWidthMax + ImageSize + 2 * Split, ScoreSingleBoxHeight, 5.0f, IGraphics::CORNER_L);
				}
				Graphics()->TextureClear();
				SetHudColor(1.0f, 1.0f, 1.0f, 1.0f);
				if(m_aScoreInfo[t].m_RoundRectQuadContainerIndex != -1)
					Graphics()->RenderQuadContainer(m_aScoreInfo[t].m_RoundRectQuadContainerIndex, -1);

				if(aRecreateTeamScore[t])
				{
					CTextCursor Cursor;
					Cursor.SetPosition(vec2(m_Width - ScoreWidthMax + (ScoreWidthMax - m_aScoreInfo[t].m_ScoreTextWidth) / 2 - Split, StartY + t * 20 + (18.f - 14.f) / 2.f));
					Cursor.m_FontSize = 14.0f;
					TextRender()->RecreateTextContainer(m_aScoreInfo[t].m_TextScoreContainerIndex, &Cursor, aScoreTeam[t]);
				}
				if(m_aScoreInfo[t].m_TextScoreContainerIndex.Valid())
				{
					ColorRGBA TColor(1.f, 1.f, 1.f, 1.f);
					ColorRGBA TOutlineColor(0.f, 0.f, 0.f, 0.3f);
					TextRender()->RenderTextContainer(m_aScoreInfo[t].m_TextScoreContainerIndex, ApplyHudAlpha(TColor), ApplyHudAlpha(TOutlineColor));
				}

				if(GameClient()->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS)
				{
					int BlinkTimer = (GameClient()->m_aFlagDropTick[t] != 0 &&
								 (Client()->GameTick(g_Config.m_ClDummy) - GameClient()->m_aFlagDropTick[t]) / Client()->GameTickSpeed() >= 25) ?
								 10 :
								 20;
					if(aFlagCarrier[t] == FLAG_ATSTAND || (aFlagCarrier[t] == FLAG_TAKEN && ((Client()->GameTick(g_Config.m_ClDummy) / BlinkTimer) & 1)))
					{
						Graphics()->TextureSet(t == 0 ? GameClient()->m_GameSkin.m_SpriteFlagRed : GameClient()->m_GameSkin.m_SpriteFlagBlue);
						SetHudColor(1.f, 1.f, 1.f, 1.f);
						Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_FlagOffset, m_Width - ScoreWidthMax - ImageSize, StartY + 1.0f + t * 20);
					}
					else if(aFlagCarrier[t] >= 0)
					{
						int Id = aFlagCarrier[t] % MAX_CLIENTS;
						const char *pName = GameClient()->m_aClients[Id].m_aName;
						if(str_comp(pName, m_aScoreInfo[t].m_aPlayerNameText) != 0 || RecreateRect)
						{
							str_copy(m_aScoreInfo[t].m_aPlayerNameText, pName);

							float w = TextRender()->TextWidth(8.0f, pName, -1, -1.0f);

							CTextCursor Cursor;
							Cursor.SetPosition(vec2(minimum(m_Width - w - 1.0f, m_Width - ScoreWidthMax - ImageSize - 2 * Split), StartY + (t + 1) * 20.0f - 2.0f));
							Cursor.m_FontSize = 8.0f;
							TextRender()->RecreateTextContainer(m_aScoreInfo[t].m_OptionalNameTextContainerIndex, &Cursor, pName);
						}

						if(m_aScoreInfo[t].m_OptionalNameTextContainerIndex.Valid())
						{
							ColorRGBA TColor(1.f, 1.f, 1.f, 1.f);
							ColorRGBA TOutlineColor(0.f, 0.f, 0.f, 0.3f);
							TextRender()->RenderTextContainer(m_aScoreInfo[t].m_OptionalNameTextContainerIndex, ApplyHudAlpha(TColor), ApplyHudAlpha(TOutlineColor));
						}

						CTeeRenderInfo TeeInfo = GameClient()->m_aClients[Id].m_RenderInfo;
						TeeInfo.m_Size = ScoreSingleBoxHeight;

						const CAnimState *pIdleState = CAnimState::GetIdle();
						vec2 OffsetToMid;
						CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
						vec2 TeeRenderPos(m_Width - ScoreWidthMax - TeeInfo.m_Size / 2 - Split, StartY + (t * 20) + ScoreSingleBoxHeight / 2.0f + OffsetToMid.y);

						RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);
					}
				}
				StartY += 8.0f;
			}
		}
		else
		{
			int Local = -1;
			int aPos[2] = {1, 2};
			const CNetObj_PlayerInfo *apPlayerInfo[2] = {nullptr, nullptr};
			int i = 0;
			for(int t = 0; t < 2 && i < MAX_CLIENTS && GameClient()->m_Snap.m_apInfoByScore[i]; ++i)
			{
				if(GameClient()->m_Snap.m_apInfoByScore[i]->m_Team != TEAM_SPECTATORS)
				{
					apPlayerInfo[t] = GameClient()->m_Snap.m_apInfoByScore[i];
					if(apPlayerInfo[t]->m_ClientId == GameClient()->m_Snap.m_LocalClientId)
						Local = t;
					++t;
				}
			}
			if(Local == -1 && GameClient()->m_Snap.m_pLocalInfo && GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
			{
				for(; i < MAX_CLIENTS && GameClient()->m_Snap.m_apInfoByScore[i]; ++i)
				{
					if(GameClient()->m_Snap.m_apInfoByScore[i]->m_Team != TEAM_SPECTATORS)
						++aPos[1];
					if(GameClient()->m_Snap.m_apInfoByScore[i]->m_ClientId == GameClient()->m_Snap.m_LocalClientId)
					{
						apPlayerInfo[1] = GameClient()->m_Snap.m_apInfoByScore[i];
						Local = 1;
						break;
					}
				}
			}
			char aScore[2][16];
			for(int t = 0; t < 2; ++t)
			{
				if(apPlayerInfo[t])
				{
					if(Client()->IsSixup() && GameClient()->m_Snap.m_pGameInfoObj->m_GameFlags & protocol7::GAMEFLAG_RACE)
						str_time((int64_t)absolute(apPlayerInfo[t]->m_Score) / 10, TIME_MINS_CENTISECS, aScore[t], sizeof(aScore[t]));
					else if(GameClient()->m_GameInfo.m_TimeScore)
					{
						if(apPlayerInfo[t]->m_Score != -9999)
							str_time((int64_t)absolute(apPlayerInfo[t]->m_Score) * 100, TIME_HOURS, aScore[t], sizeof(aScore[t]));
						else
							aScore[t][0] = 0;
					}
					else
						str_format(aScore[t], sizeof(aScore[t]), "%d", apPlayerInfo[t]->m_Score);
				}
				else
					aScore[t][0] = 0;
			}

			bool RecreateScores = str_comp(aScore[0], m_aScoreInfo[0].m_aScoreText) != 0 || str_comp(aScore[1], m_aScoreInfo[1].m_aScoreText) != 0 || m_LastLocalClientId != GameClient()->m_Snap.m_LocalClientId;
			m_LastLocalClientId = GameClient()->m_Snap.m_LocalClientId;

			bool RecreateRect = ForceScoreInfoInit;
			for(int t = 0; t < 2; t++)
			{
				if(RecreateScores)
				{
					m_aScoreInfo[t].m_ScoreTextWidth = TextRender()->TextWidth(14.0f, aScore[t], -1, -1.0f);
					str_copy(m_aScoreInfo[t].m_aScoreText, aScore[t]);
					RecreateRect = true;
				}

				if(apPlayerInfo[t])
				{
					int Id = apPlayerInfo[t]->m_ClientId;
					if(Id >= 0 && Id < MAX_CLIENTS)
					{
						const char *pName = GameClient()->m_aClients[Id].m_aName;
						if(str_comp(pName, m_aScoreInfo[t].m_aPlayerNameText) != 0)
							RecreateRect = true;
					}
				}
				else
				{
					if(m_aScoreInfo[t].m_aPlayerNameText[0] != 0)
						RecreateRect = true;
				}

				char aBuf[16];
				str_format(aBuf, sizeof(aBuf), "%d.", aPos[t]);
				if(str_comp(aBuf, m_aScoreInfo[t].m_aRankText) != 0)
					RecreateRect = true;
			}

			static float s_TextWidth10 = TextRender()->TextWidth(14.0f, "10", -1, -1.0f);
			float ScoreWidthMax = maximum(maximum(m_aScoreInfo[0].m_ScoreTextWidth, m_aScoreInfo[1].m_ScoreTextWidth), s_TextWidth10);
			float Split = 3.0f, ImageSize = 16.0f, PosSize = 16.0f;

			for(int t = 0; t < 2; t++)
			{
				if(RecreateRect)
				{
					Graphics()->DeleteQuadContainer(m_aScoreInfo[t].m_RoundRectQuadContainerIndex);

					if(t == Local)
						SetHudColor(1.0f, 1.0f, 1.0f, 0.25f);
					else
						SetHudColor(0.0f, 0.0f, 0.0f, 0.25f);
					m_aScoreInfo[t].m_RoundRectQuadContainerIndex = Graphics()->CreateRectQuadContainer(m_Width - ScoreWidthMax - ImageSize - 2 * Split - PosSize, StartY + t * 20, ScoreWidthMax + ImageSize + 2 * Split + PosSize, ScoreSingleBoxHeight, 5.0f, IGraphics::CORNER_L);
				}
				Graphics()->TextureClear();
				SetHudColor(1.0f, 1.0f, 1.0f, 1.0f);
				if(m_aScoreInfo[t].m_RoundRectQuadContainerIndex != -1)
					Graphics()->RenderQuadContainer(m_aScoreInfo[t].m_RoundRectQuadContainerIndex, -1);

				if(RecreateScores)
				{
					CTextCursor Cursor;
					Cursor.SetPosition(vec2(m_Width - ScoreWidthMax + (ScoreWidthMax - m_aScoreInfo[t].m_ScoreTextWidth) - Split, StartY + t * 20 + (18.f - 14.f) / 2.f));
					Cursor.m_FontSize = 14.0f;
					TextRender()->RecreateTextContainer(m_aScoreInfo[t].m_TextScoreContainerIndex, &Cursor, aScore[t]);
				}
				if(m_aScoreInfo[t].m_TextScoreContainerIndex.Valid())
				{
					ColorRGBA TColor(1.f, 1.f, 1.f, 1.f);
					ColorRGBA TOutlineColor(0.f, 0.f, 0.f, 0.3f);
					TextRender()->RenderTextContainer(m_aScoreInfo[t].m_TextScoreContainerIndex, ApplyHudAlpha(TColor), ApplyHudAlpha(TOutlineColor));
				}

				if(apPlayerInfo[t])
				{
					int Id = apPlayerInfo[t]->m_ClientId;
					if(Id >= 0 && Id < MAX_CLIENTS)
					{
						const char *pName = GameClient()->m_aClients[Id].m_aName;
						if(RecreateRect)
						{
							str_copy(m_aScoreInfo[t].m_aPlayerNameText, pName);

							CTextCursor Cursor;
							Cursor.SetPosition(vec2(minimum(m_Width - TextRender()->TextWidth(8.0f, pName) - 1.0f, m_Width - ScoreWidthMax - ImageSize - 2 * Split - PosSize), StartY + (t + 1) * 20.0f - 2.0f));
							Cursor.m_FontSize = 8.0f;
							TextRender()->RecreateTextContainer(m_aScoreInfo[t].m_OptionalNameTextContainerIndex, &Cursor, pName);
						}

						if(m_aScoreInfo[t].m_OptionalNameTextContainerIndex.Valid())
						{
							ColorRGBA TColor(1.f, 1.f, 1.f, 1.f);
							ColorRGBA TOutlineColor(0.f, 0.f, 0.f, 0.3f);
							TextRender()->RenderTextContainer(m_aScoreInfo[t].m_OptionalNameTextContainerIndex, ApplyHudAlpha(TColor), ApplyHudAlpha(TOutlineColor));
						}

						CTeeRenderInfo TeeInfo = GameClient()->m_aClients[Id].m_RenderInfo;
						TeeInfo.m_Size = ScoreSingleBoxHeight;

						const CAnimState *pIdleState = CAnimState::GetIdle();
						vec2 OffsetToMid;
						CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
						vec2 TeeRenderPos(m_Width - ScoreWidthMax - TeeInfo.m_Size / 2 - Split, StartY + (t * 20) + ScoreSingleBoxHeight / 2.0f + OffsetToMid.y);

						RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);
					}
				}
				else
				{
					m_aScoreInfo[t].m_aPlayerNameText[0] = 0;
				}

				char aBuf[16];
				str_format(aBuf, sizeof(aBuf), "%d.", aPos[t]);
				if(RecreateRect)
				{
					str_copy(m_aScoreInfo[t].m_aRankText, aBuf);

					CTextCursor Cursor;
					Cursor.SetPosition(vec2(m_Width - ScoreWidthMax - ImageSize - Split - PosSize, StartY + t * 20 + (18.f - 10.f) / 2.f));
					Cursor.m_FontSize = 10.0f;
					TextRender()->RecreateTextContainer(m_aScoreInfo[t].m_TextRankContainerIndex, &Cursor, aBuf);
				}
				if(m_aScoreInfo[t].m_TextRankContainerIndex.Valid())
				{
					ColorRGBA TColor(1.f, 1.f, 1.f, 1.f);
					ColorRGBA TOutlineColor(0.f, 0.f, 0.f, 0.3f);
					TextRender()->RenderTextContainer(m_aScoreInfo[t].m_TextRankContainerIndex, ApplyHudAlpha(TColor), ApplyHudAlpha(TOutlineColor));
				}

				StartY += 8.0f;
			}
		}
	}
}

void CHud::RenderWarmupTimer()
{
	// render warmup timer
	const bool Visible = GameClient()->m_Snap.m_pGameInfoObj->m_WarmupTimer > 0 &&
		(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_RACETIME) == 0;
	static float s_Phase = 0.0f;
	const float Dt = Client()->RenderFrameTime();
	if(BCUiAnimations::Enabled())
		BCUiAnimations::UpdatePhase(s_Phase, Visible ? 1.0f : 0.0f, Dt, 0.22f);
	else
		s_Phase = Visible ? 1.0f : 0.0f;
	if(s_Phase <= 0.0f)
		return;

	const float Ease = BCUiAnimations::EaseInOutQuad(s_Phase);
	const float Alpha = Ease;
	const float YOff = -10.0f * (1.0f - Ease);

	char aBuf[256];
	float FontSize = 20.0f;
	float w = TextRender()->TextWidth(FontSize, Localize("Warmup"), -1, -1.0f);
	SetHudTextColor(1.0f, 1.0f, 1.0f, Alpha);
	TextRender()->Text(150 * Graphics()->ScreenAspect() + -w / 2, 50 + YOff, FontSize, Localize("Warmup"), -1.0f);

	int Seconds = GameClient()->m_Snap.m_pGameInfoObj->m_WarmupTimer / Client()->GameTickSpeed();
	if(Seconds < 5)
		str_format(aBuf, sizeof(aBuf), "%d.%d", Seconds, (GameClient()->m_Snap.m_pGameInfoObj->m_WarmupTimer * 10 / Client()->GameTickSpeed()) % 10);
	else
		str_format(aBuf, sizeof(aBuf), "%d", Seconds);
	w = TextRender()->TextWidth(FontSize, aBuf, -1, -1.0f);
	TextRender()->Text(150 * Graphics()->ScreenAspect() + -w / 2, 75 + YOff, FontSize, aBuf, -1.0f);
	SetHudTextColor(TextRender()->DefaultTextColor());
}

void CHud::RenderTextInfo(bool ForcePreview)
{
	int Showfps = g_Config.m_ClShowfps || ForcePreview;
#if defined(CONF_VIDEORECORDER)
	if(IVideo::Current())
		Showfps = 0;
#endif
	if(Showfps)
	{
		const auto Layout = HudLayout::Get(HudLayout::MODULE_FPS, m_Width, m_Height);
		const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
		const float FontSize = 12.0f * Scale;
		const float PosX = Layout.m_X;
		const float PosY = Layout.m_Y;
		const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
		const unsigned BackgroundColor = Layout.m_BackgroundColor;
		char aBuf[16];
		const int FramesPerSecond = round_to_int(1.0f / Client()->FrameTimeAverage());
		str_format(aBuf, sizeof(aBuf), "%d", FramesPerSecond);

		static float s_FontSize = -1.0f;
		static float s_aTextWidth[5] = {};
		if(s_FontSize != FontSize)
		{
			s_FontSize = FontSize;
			s_aTextWidth[0] = TextRender()->TextWidth(FontSize, "0", -1, -1.0f);
			s_aTextWidth[1] = TextRender()->TextWidth(FontSize, "00", -1, -1.0f);
			s_aTextWidth[2] = TextRender()->TextWidth(FontSize, "000", -1, -1.0f);
			s_aTextWidth[3] = TextRender()->TextWidth(FontSize, "0000", -1, -1.0f);
			s_aTextWidth[4] = TextRender()->TextWidth(FontSize, "00000", -1, -1.0f);
		}

		int DigitIndex = GetDigitsIndex(FramesPerSecond, 4);
		float DrawX = std::clamp(PosX, 0.0f, maximum(0.0f, m_Width - s_aTextWidth[DigitIndex]));
		const float DrawY = std::clamp(PosY, 0.0f, maximum(0.0f, m_Height - FontSize));
		const CUIRect PushRect = {DrawX - 2.0f * Scale, DrawY - 1.0f * Scale, s_aTextWidth[DigitIndex] + 4.0f * Scale, FontSize + 2.0f * Scale};
		DrawX = std::clamp(DrawX + GameClient()->m_MusicPlayer.GetHudPushOffsetForRect(PushRect, m_Width, 4.0f * Scale), 0.0f, maximum(0.0f, m_Width - s_aTextWidth[DigitIndex]));
		if(BackgroundEnabled)
		{
			const float RectX = DrawX - 2.0f * Scale;
			const float RectY = DrawY - 1.0f * Scale;
			const float RectW = s_aTextWidth[DigitIndex] + 4.0f * Scale;
			const float RectH = FontSize + 2.0f * Scale;
			const int Corners = HudModuleBackgroundCorners(IGraphics::CORNER_ALL, RectX, RectY, RectW, RectH, m_Width, m_Height);
			Graphics()->DrawRect(RectX, RectY, RectW, RectH, ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)), Corners, 2.0f * Scale);
		}

		CTextCursor Cursor;
		Cursor.SetPosition(vec2(DrawX, DrawY));
		Cursor.m_FontSize = FontSize;
		auto OldFlags = TextRender()->GetRenderFlags();
		TextRender()->SetRenderFlags(OldFlags | TEXT_RENDER_FLAG_ONE_TIME_USE);
		if(m_FPSTextContainerIndex.Valid())
			TextRender()->RecreateTextContainerSoft(m_FPSTextContainerIndex, &Cursor, aBuf);
		else
			TextRender()->CreateTextContainer(m_FPSTextContainerIndex, &Cursor, "0");
		TextRender()->SetRenderFlags(OldFlags);
		if(m_FPSTextContainerIndex.Valid())
		{
			TextRender()->RenderTextContainer(m_FPSTextContainerIndex, ApplyHudAlpha(TextRender()->DefaultTextColor()), ApplyHudAlpha(TextRender()->DefaultTextOutlineColor()));
		}
	}

	if((g_Config.m_ClShowpred && Client()->State() != IClient::STATE_DEMOPLAYBACK) || ForcePreview)
	{
		const auto Layout = HudLayout::Get(HudLayout::MODULE_PING, m_Width, m_Height);
		const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
		const float FontSize = 12.0f * Scale;
		const float PosX = Layout.m_X;
		const float PosY = Layout.m_Y;
		const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
		const unsigned BackgroundColor = Layout.m_BackgroundColor;
		char aBuf[16];
		const int PredictionTime = ForcePreview ? 36 : std::clamp(Client()->GetPredictionTime(), 0, 9999);
		str_format(aBuf, sizeof(aBuf), "%d", PredictionTime);
		const float Width = TextRender()->TextWidth(FontSize, aBuf, -1, -1.0f);
		float DrawX = std::clamp(PosX, 0.0f, maximum(0.0f, m_Width - Width));
		const float DrawY = std::clamp(PosY, 0.0f, maximum(0.0f, m_Height - FontSize));
		const CUIRect PushRect = {DrawX - 2.0f * Scale, DrawY - 1.0f * Scale, Width + 4.0f * Scale, FontSize + 2.0f * Scale};
		DrawX = std::clamp(DrawX + GameClient()->m_MusicPlayer.GetHudPushOffsetForRect(PushRect, m_Width, 4.0f * Scale), 0.0f, maximum(0.0f, m_Width - Width));
		if(BackgroundEnabled)
		{
			const float RectX = DrawX - 2.0f * Scale;
			const float RectY = DrawY - 1.0f * Scale;
			const float RectW = Width + 4.0f * Scale;
			const float RectH = FontSize + 2.0f * Scale;
			const int Corners = HudModuleBackgroundCorners(IGraphics::CORNER_ALL, RectX, RectY, RectW, RectH, m_Width, m_Height);
			Graphics()->DrawRect(RectX, RectY, RectW, RectH, ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)), Corners, 2.0f * Scale);
		}
		TextRender()->Text(DrawX, DrawY, FontSize, aBuf, -1.0f);
	}

	if(!ForcePreview && g_Config.m_TcMiniDebug)
	{
		float FontSize = 8.0f;
		float TextHeight = 11.0f;
		char aBuf[64];
		float OffsetY = 3.0f;

		int PlayerId = GameClient()->m_Snap.m_LocalClientId;
		if(GameClient()->m_Snap.m_SpecInfo.m_Active)
			PlayerId = GameClient()->m_Snap.m_SpecInfo.m_SpectatorId;

		if(g_Config.m_ClShowhudDDRace && GameClient()->m_Snap.m_aCharacters[PlayerId].m_HasExtendedData && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW)
			OffsetY += 50.0f;
		else if(g_Config.m_ClShowhudHealthAmmo && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW)
			OffsetY += 27.0f;

		vec2 Pos;
		if(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW)
			Pos = vec2(GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].x, GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].y);
		else
			Pos = GameClient()->m_aClients[PlayerId].m_RenderPos;

		str_format(aBuf, sizeof(aBuf), "X: %.2f", Pos.x / 32.0f);
		TextRender()->Text(4, OffsetY, FontSize, aBuf, -1.0f);

		OffsetY += TextHeight;
		str_format(aBuf, sizeof(aBuf), "Y: %.2f", Pos.y / 32.0f);
		TextRender()->Text(4, OffsetY, FontSize, aBuf, -1.0f);
		if(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW)
		{
			OffsetY += TextHeight;
			str_format(aBuf, sizeof(aBuf), "Angle: %d", GameClient()->m_aClients[PlayerId].m_RenderCur.m_Angle);
			TextRender()->Text(4.0f, OffsetY, FontSize, aBuf, -1.0f);

			OffsetY += TextHeight;
			str_format(aBuf, sizeof(aBuf), "VelY: %.2f", GameClient()->m_Snap.m_aCharacters[PlayerId].m_Cur.m_VelY / 256.0f * 50.0f / 32.0f);
			TextRender()->Text(4.0f, OffsetY, FontSize, aBuf, -1.0f);

			OffsetY += TextHeight;

			str_format(aBuf, sizeof(aBuf), "VelX: %.2f", GameClient()->m_Snap.m_aCharacters[PlayerId].m_Cur.m_VelX / 256.0f * 50.0f / 32.0f);
			TextRender()->Text(4.0f, OffsetY, FontSize, aBuf, -1.0f);
		}
	}
	if(!ForcePreview && g_Config.m_TcRenderCursorSpec && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW)
	{
		int CurWeapon = 1;
		SetHudColor(1.f, 1.f, 1.f, g_Config.m_TcRenderCursorSpecAlpha / 100.0f);
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_aSpriteWeaponCursors[CurWeapon]);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_aCursorOffset[CurWeapon], m_Width / 2.0f, m_Height / 2.0f, 0.36f, 0.36f);
	}
	// render team in freeze text and last notify
	const bool FrozenTeeDisplayDisabled = GameClient()->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_GORES_FROZEN_TEE_DISPLAY);
	const bool HudComponentDisabled = GameClient()->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_GORES_HUD);
	const bool ShowFrozenText = !FrozenTeeDisplayDisabled && g_Config.m_TcShowFrozenText > 0;
	const bool ShowFrozenHud = !FrozenTeeDisplayDisabled && g_Config.m_TcShowFrozenHud > 0;
	const bool ShowLastAliveNotify = !HudComponentDisabled && g_Config.m_TcNotifyWhenLast;
	if((ShowFrozenText || ShowFrozenHud || ShowLastAliveNotify || ForcePreview) && (GameClient()->m_GameInfo.m_EntitiesDDRace || ForcePreview))
	{
		int NumInTeam = 0;
		int NumFrozen = 0;
		int LocalTeamID = 0;
		bool HasLiveTeamData = false;
		if(GameClient()->m_GameInfo.m_EntitiesDDRace && GameClient()->m_Snap.m_LocalClientId >= 0 && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId >= 0)
		{
			if(GameClient()->m_Snap.m_SpecInfo.m_Active == 1 && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != -1)
				LocalTeamID = GameClient()->m_Teams.Team(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId);
			else
				LocalTeamID = GameClient()->m_Teams.Team(GameClient()->m_Snap.m_LocalClientId);
		}
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameClient()->m_Snap.m_apPlayerInfos[i])
				continue;

			if(GameClient()->m_Teams.Team(i) == LocalTeamID)
			{
				NumInTeam++;
				if(GameClient()->m_aClients[i].m_FreezeEnd > 0 || GameClient()->m_aClients[i].m_DeepFrozen)
					NumFrozen++;
			}
		}
		HasLiveTeamData = NumInTeam > 0;
		if(!HasLiveTeamData && ForcePreview)
		{
			NumInTeam = 5;
			NumFrozen = 2;
		}

		// Notify when last
		if(ShowLastAliveNotify || ForcePreview)
		{
			if((NumInTeam > 1 && NumInTeam - NumFrozen == 1) || ForcePreview)
			{
				SetHudTextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_TcNotifyWhenLastColor)));
				const float FontSize = g_Config.m_TcNotifyWhenLastSize;
				const char *pNotifyText = g_Config.m_TcNotifyWhenLastText[0] != '\0' ? g_Config.m_TcNotifyWhenLastText : "YOU ARE LAST";
				float XPos = (g_Config.m_TcNotifyWhenLastX / 100.0f) * m_Width;
				float YPos = (g_Config.m_TcNotifyWhenLastY / 100.0f) * m_Height;
				XPos = std::clamp(XPos, 0.0f, maximum(0.0f, m_Width - FontSize));
				YPos = std::clamp(YPos, 0.0f, maximum(0.0f, m_Height - FontSize));

				TextRender()->Text(XPos, YPos, FontSize, pNotifyText, -1.0f);
				SetHudTextColor(TextRender()->DefaultTextColor());

				if(!ForcePreview && g_Config.m_BcNotifyWhenLastSound){
					// ВАХВАХВАХВАХ ПИЗДЕЦ ЧТО ЗА ЗВУК
					GameClient()->m_Sounds.Play(CSounds::CHN_WORLD, SOUND_CTF_DROP, 1.0f);
				}
			}
		}
		// Show freeze text
		char aBuf[64];
		if(!ForcePreview && ShowFrozenText && g_Config.m_TcShowFrozenText == 1)
			str_format(aBuf, sizeof(aBuf), "%d / %d", NumInTeam - NumFrozen, NumInTeam);
		else if(!ForcePreview && ShowFrozenText && g_Config.m_TcShowFrozenText == 2)
			str_format(aBuf, sizeof(aBuf), "%d / %d", NumFrozen, NumInTeam);
		if(!ForcePreview && ShowFrozenText)
			TextRender()->Text(m_Width / 2.0f - TextRender()->TextWidth(10.0f, aBuf) / 2.0f, 12.0f, 10.0f, aBuf);

		// str_format(aBuf, sizeof(aBuf), "%d", GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_PrevPredicted.m_FreezeEnd);
		// str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_ClWhatsMyPing);
		// TextRender()->Text(0, m_Width / 2 - TextRender()->TextWidth(0, 10, aBuf, -1, -1.0f) / 2, 20, 10, aBuf, -1.0f);

		if(ShowFrozenHud && (!GameClient()->m_Scoreboard.IsActive() || ForcePreview) && (!g_Config.m_TcFrozenHudTeamOnly || LocalTeamID != 0 || ForcePreview))
		{
			const auto FrozenLayout = HudLayout::Get(HudLayout::MODULE_FROZEN_HUD, m_Width, m_Height);
			CTeeRenderInfo FreezeInfo;
			const CSkin *pSkin = GameClient()->m_Skins.Find("x_ninja");
			FreezeInfo.m_OriginalRenderSkin = pSkin->m_OriginalSkin;
			FreezeInfo.m_ColorableRenderSkin = pSkin->m_ColorableSkin;
			FreezeInfo.m_BloodColor = pSkin->m_BloodColor;
			FreezeInfo.m_SkinMetrics = pSkin->m_Metrics;
			FreezeInfo.m_ColorBody = ColorRGBA(1.0f, 1.0f, 1.0f);
			FreezeInfo.m_ColorFeet = ColorRGBA(1.0f, 1.0f, 1.0f);
			FreezeInfo.m_CustomColoredSkin = false;

			const float HudScale = std::clamp(FrozenLayout.m_Scale / 100.0f, 0.25f, 3.0f);
			float progressiveOffset = 0.0f;
			float TeeSize = g_Config.m_TcFrozenHudTeeSize * HudScale;
			int MaxTees = (int)(8.3f * (m_Width / m_Height) * 13.0f / TeeSize);
			if(!g_Config.m_ClShowfps && !g_Config.m_ClShowpred)
				MaxTees = (int)(9.5f * (m_Width / m_Height) * 13.0f / TeeSize);
			int MaxRows = g_Config.m_TcFrozenMaxRows;
			const float StartX = FrozenLayout.m_X;
			float StartY = FrozenLayout.m_Y;
			const bool BackgroundEnabled = FrozenLayout.m_BackgroundEnabled;
			const unsigned BackgroundColor = FrozenLayout.m_BackgroundColor;

			int TotalRows = std::min(MaxRows, (NumInTeam + MaxTees - 1) / MaxTees);
			if(BackgroundEnabled)
			{
				Graphics()->TextureClear();
				Graphics()->QuadsBegin();
				Graphics()->SetColor(ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)));
				const float RectX = StartX - TeeSize / 2.0f;
				const float RectY = StartY;
				const float RectW = TeeSize * std::min(NumInTeam, MaxTees);
				const float RectH = TeeSize + 3.0f + (TotalRows - 1) * TeeSize;
				const int Corners = HudModuleBackgroundCorners(IGraphics::CORNER_B, RectX, RectY, RectW, RectH, m_Width, m_Height);
				Graphics()->DrawRectExt(RectX, RectY, RectW, RectH, 5.0f, Corners);
				Graphics()->QuadsEnd();
			}

			bool Overflow = NumInTeam > MaxTees * MaxRows;

			int NumDisplayed = 0;
			int NumInRow = 0;
			int CurrentRow = 0;

			if(ForcePreview && !HasLiveTeamData)
			{
				CTeeRenderInfo PreviewLiveInfo = FreezeInfo;
				const int PreviewClientId = GameClient()->m_Snap.m_LocalClientId;
				if(PreviewClientId >= 0 && PreviewClientId < MAX_CLIENTS && GameClient()->m_aClients[PreviewClientId].m_RenderInfo.Valid())
					PreviewLiveInfo = GameClient()->m_aClients[PreviewClientId].m_RenderInfo;
				for(int i = 0; i < NumInTeam; ++i)
				{
					const bool Frozen = i < NumFrozen;
					CTeeRenderInfo TeeInfo = Frozen ? FreezeInfo : PreviewLiveInfo;
					if(!TeeInfo.Valid())
						TeeInfo = FreezeInfo;
					TeeInfo.m_Size = TeeSize;
					const CAnimState *pIdleState = CAnimState::GetIdle();
					vec2 OffsetToMid;
					CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
					vec2 TeeRenderPos(StartX + progressiveOffset, StartY + TeeSize * 0.7f + CurrentRow * TeeSize);
					if(Frozen)
						RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_PAIN, vec2(1.0f, 0.0f), TeeRenderPos, 0.8f);
					else
						RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);
					progressiveOffset += TeeSize;
					NumInRow++;
					if(NumInRow >= MaxTees)
					{
						NumInRow = 0;
						progressiveOffset = 0.0f;
						CurrentRow++;
					}
				}
				return;
			}

			for(int OverflowIndex = 0; OverflowIndex < 1 + Overflow; OverflowIndex++)
			{
				for(int i = 0; i < MAX_CLIENTS && NumDisplayed < MaxTees * MaxRows; i++)
				{
					if(!GameClient()->m_Snap.m_apPlayerInfos[i])
						continue;
					if(GameClient()->m_Teams.Team(i) == LocalTeamID)
					{
						bool Frozen = false;
						CTeeRenderInfo TeeInfo = GameClient()->m_aClients[i].m_RenderInfo;
						if(GameClient()->m_aClients[i].m_FreezeEnd > 0 || GameClient()->m_aClients[i].m_DeepFrozen)
						{
							if(!g_Config.m_TcShowFrozenHudSkins)
								TeeInfo = FreezeInfo;
							Frozen = true;
						}

						if(Overflow && Frozen && OverflowIndex == 0)
							continue;
						if(Overflow && !Frozen && OverflowIndex == 1)
							continue;

						NumDisplayed++;
						NumInRow++;
						if(NumInRow > MaxTees)
						{
							NumInRow = 1;
							progressiveOffset = 0.0f;
							CurrentRow++;
						}

						TeeInfo.m_Size = TeeSize;
						const CAnimState *pIdleState = CAnimState::GetIdle();
						vec2 OffsetToMid;
						CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
						vec2 TeeRenderPos(StartX + progressiveOffset, StartY + TeeSize * 0.7f + CurrentRow * TeeSize);
						float Alpha = 1.0f;
						CNetObj_Character CurChar = GameClient()->m_aClients[i].m_RenderCur;
						if(g_Config.m_TcShowFrozenHudSkins && Frozen)
						{
							Alpha = 0.6f;
							TeeInfo.m_ColorBody.r *= 0.4f;
							TeeInfo.m_ColorBody.g *= 0.4f;
							TeeInfo.m_ColorBody.b *= 0.4f;
							TeeInfo.m_ColorFeet.r *= 0.4f;
							TeeInfo.m_ColorFeet.g *= 0.4f;
							TeeInfo.m_ColorFeet.b *= 0.4f;
						}
						if(Frozen)
							RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_PAIN, vec2(1.0f, 0.0f), TeeRenderPos, Alpha);
						else
							RenderTools()->RenderTee(pIdleState, &TeeInfo, CurChar.m_Emote, vec2(1.0f, 0.0f), TeeRenderPos);
						progressiveOffset += TeeSize;
					}
				}
			}
		}
	}
}

void CHud::RenderLockCam(bool ForcePreview)
{
	const bool HasWorldSnapshot = Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK;
	if(!HasWorldSnapshot && !ForcePreview)
		return;
	if(GameClient()->m_Menus.IsActive() && !ForcePreview)
		return;

	const auto Layout = HudLayout::Get(HudLayout::MODULE_LOCK_CAM, m_Width, m_Height);

	int ClientId = GameClient()->m_LockCamClientId;
	if(ForcePreview && (ClientId < 0 || ClientId >= MAX_CLIENTS || !GameClient()->m_aClients[ClientId].m_Active))
	{
		ClientId = GameClient()->m_Snap.m_LocalClientId;
		if(ClientId < 0 || ClientId >= MAX_CLIENTS || !GameClient()->m_aClients[ClientId].m_Active)
		{
			for(int Index = 0; Index < MAX_CLIENTS; ++Index)
			{
				if(GameClient()->m_aClients[Index].m_Active)
				{
					ClientId = Index;
					break;
				}
			}
		}
	}
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
	{
		if(!ForcePreview)
			return;
		ClientId = -1;
	}

	const CGameClient::CClientData *pClientData = nullptr;
	if(ClientId >= 0 && ClientId < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = GameClient()->m_aClients[ClientId];
		if(ClientData.m_Active && ClientData.m_RenderInfo.Valid())
			pClientData = &ClientData;
		else if(!ForcePreview)
		{
			GameClient()->m_LockCamClientId = -1;
			return;
		}
	}

	vec2 CameraOffset(0.0f, 0.0f);
	if(ClientId == GameClient()->m_aLocalIds[0])
		CameraOffset = GameClient()->m_Camera.m_aDyncamCurrentCameraOffset[0];
	else if(Client()->DummyConnected() && ClientId == GameClient()->m_aLocalIds[1])
		CameraOffset = GameClient()->m_Camera.m_aDyncamCurrentCameraOffset[1];

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
	const float ViewHeight = 120.0f * Scale;
	const float ViewWidth = ViewHeight * Graphics()->ScreenAspect();
	float DrawX = Layout.m_X - ViewWidth * 0.5f;
	float DrawY = Layout.m_Y;
	DrawX = std::clamp(DrawX, 0.0f, maximum(0.0f, m_Width - ViewWidth));
	DrawY = std::clamp(DrawY, 0.0f, maximum(0.0f, m_Height - ViewHeight));

	CUIRect ViewRect;
	ViewRect.x = DrawX;
	ViewRect.y = DrawY;
	ViewRect.w = ViewWidth;
	ViewRect.h = ViewHeight;

	const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
	const unsigned BackgroundColor = Layout.m_BackgroundColor;
	const float Border = 2.0f * Scale;
	const ColorRGBA BorderColor = BackgroundEnabled ? ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)) : ApplyHudAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.6f));
	const ColorRGBA InnerColor = BackgroundEnabled ? ApplyHudAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.15f)) : ApplyHudAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f));
	const int OuterCorners = HudModuleBackgroundCorners(IGraphics::CORNER_ALL, ViewRect.x - Border, ViewRect.y - Border, ViewRect.w + Border * 2.0f, ViewRect.h + Border * 2.0f, m_Width, m_Height);
	const int InnerCorners = HudModuleBackgroundCorners(IGraphics::CORNER_ALL, ViewRect.x, ViewRect.y, ViewRect.w, ViewRect.h, m_Width, m_Height);
	Graphics()->DrawRect(ViewRect.x - Border, ViewRect.y - Border, ViewRect.w + Border * 2.0f, ViewRect.h + Border * 2.0f,
		BorderColor, OuterCorners, 4.0f * Scale + Border);
	Graphics()->DrawRect(ViewRect.x, ViewRect.y, ViewRect.w, ViewRect.h,
		InnerColor, InnerCorners, 4.0f * Scale);

	const float XScale = Graphics()->ScreenWidth() / m_Width;
	const float YScale = Graphics()->ScreenHeight() / m_Height;
	const int ViewX = (int)std::round(ViewRect.x * XScale);
	const int ViewY = (int)std::round((m_Height - (ViewRect.y + ViewRect.h)) * YScale);
	const int ViewW = (int)std::round(ViewRect.w * XScale);
	const int ViewH = (int)std::round(ViewRect.h * YScale);
	if(ViewW <= 0 || ViewH <= 0)
	{
		Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
		return;
	}

	if(ForcePreview)
	{
		const ColorRGBA PlaceholderColor = ApplyHudAlpha(ColorRGBA(0.05f, 0.08f, 0.10f, 0.82f));
		Graphics()->DrawRect(ViewRect.x + 3.0f * Scale, ViewRect.y + 3.0f * Scale, ViewRect.w - 6.0f * Scale, ViewRect.h - 6.0f * Scale, PlaceholderColor, IGraphics::CORNER_ALL, 4.0f * Scale);

		Graphics()->LinesBegin();
		Graphics()->SetColor(0.85f, 0.95f, 1.0f, 0.18f * HudAlpha());
		IGraphics::CLineItem aGuideLines[4] = {
			IGraphics::CLineItem(ViewRect.x + ViewRect.w * 0.5f, ViewRect.y + 10.0f * Scale, ViewRect.x + ViewRect.w * 0.5f, ViewRect.y + ViewRect.h - 10.0f * Scale),
			IGraphics::CLineItem(ViewRect.x + 10.0f * Scale, ViewRect.y + ViewRect.h * 0.5f, ViewRect.x + ViewRect.w - 10.0f * Scale, ViewRect.y + ViewRect.h * 0.5f),
			IGraphics::CLineItem(ViewRect.x + 12.0f * Scale, ViewRect.y + 12.0f * Scale, ViewRect.x + ViewRect.w - 12.0f * Scale, ViewRect.y + ViewRect.h - 12.0f * Scale),
			IGraphics::CLineItem(ViewRect.x + ViewRect.w - 12.0f * Scale, ViewRect.y + 12.0f * Scale, ViewRect.x + 12.0f * Scale, ViewRect.y + ViewRect.h - 12.0f * Scale),
		};
		Graphics()->LinesDraw(aGuideLines, std::size(aGuideLines));
		Graphics()->LinesEnd();

		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->TextColor(ApplyHudAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.85f)));
		const float IconSize = 16.0f * Scale;
		const float IconWidth = TextRender()->TextWidth(IconSize, FontIcons::FONT_ICON_CAMERA, -1, -1.0f);
		TextRender()->Text(ViewRect.x + (ViewRect.w - IconWidth) * 0.5f, ViewRect.y + (ViewRect.h - IconSize) * 0.5f - 4.0f * Scale, IconSize, FontIcons::FONT_ICON_CAMERA, -1.0f);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
		return;
	}

	if(pClientData && HasWorldSnapshot)
	{
		const vec2 LockCenter = pClientData->m_RenderPos + CameraOffset;
		const float BaseZoom = GameClient()->m_Camera.m_Zoom;
		const float LockZoom = std::clamp(g_Config.m_BcLockCamUseCustomZoom ? (g_Config.m_BcLockCamZoom / 100.0f) : (BaseZoom * 0.85f), 0.05f, 10.0f);

		const vec2 OldCenter = GameClient()->m_Camera.m_Center;
		const float OldZoom = GameClient()->m_Camera.m_Zoom;

		Graphics()->UpdateViewport(ViewX, ViewY, ViewW, ViewH, false);

		GameClient()->m_Camera.m_Center = LockCenter;
		GameClient()->m_Camera.m_Zoom = LockZoom;

		// Ensure world mapping before rendering world components.
		RenderTools()->MapScreenToGroup(LockCenter.x, LockCenter.y, GameClient()->Layers()->GameGroup(), LockZoom);

		GameClient()->m_MapLayersBackground.OnRender();
		GameClient()->m_Particles.RenderGroup(CParticles::GROUP_PROJECTILE_TRAIL);
		GameClient()->m_Particles.RenderGroup(CParticles::GROUP_TRAIL_EXTRA);
		GameClient()->m_Items.OnRender();
		GameClient()->m_Trails.OnRender();
		const vec2 OldLocalPos = GameClient()->m_LocalCharacterPos;
		GameClient()->m_LocalCharacterPos = pClientData->m_RenderPos;
		GameClient()->m_3DParticles.RenderSecondary();
		GameClient()->m_LocalCharacterPos = OldLocalPos;
		GameClient()->m_Ghost.OnRender();
		GameClient()->m_Players.OnRender();
		GameClient()->m_MapLayersForeground.OnRender();
		GameClient()->m_Outlines.OnRender();
		GameClient()->m_Pet.OnRender();
		GameClient()->m_Particles.RenderGroup(CParticles::GROUP_EXPLOSIONS);
		GameClient()->m_NamePlates.OnRender();
		GameClient()->m_Particles.RenderGroup(CParticles::GROUP_EXTRA);
		GameClient()->m_Particles.RenderGroup(CParticles::GROUP_GENERAL);
		GameClient()->m_FreezeBars.OnRender();

		GameClient()->m_Camera.m_Center = OldCenter;
		GameClient()->m_Camera.m_Zoom = OldZoom;
	}

	Graphics()->UpdateViewport(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight(), false);
	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

void CHud::RenderConnectionWarning()
{
	const bool Visible = Client()->ConnectionProblems();
	static float s_Phase = 0.0f;
	const float Dt = Client()->RenderFrameTime();
	if(BCUiAnimations::Enabled())
		BCUiAnimations::UpdatePhase(s_Phase, Visible ? 1.0f : 0.0f, Dt, 0.22f);
	else
		s_Phase = Visible ? 1.0f : 0.0f;
	if(s_Phase <= 0.0f)
		return;

	const float Ease = BCUiAnimations::EaseInOutQuad(s_Phase);
	const float Alpha = Ease;
	const float YOff = -10.0f * (1.0f - Ease);

	const char *pText = Localize("Connection Problems…");
	float w = TextRender()->TextWidth(24, pText, -1, -1.0f);
	SetHudTextColor(1.0f, 1.0f, 1.0f, Alpha);
	TextRender()->Text(150 * Graphics()->ScreenAspect() - w / 2, 50 + YOff, 24, pText, -1.0f);
	SetHudTextColor(TextRender()->DefaultTextColor());
}

void CHud::RenderTeambalanceWarning()
{
	// render prompt about team-balance
	bool Flash = time() / (time_freq() / 2) % 2 == 0;
	int TeamDiff = 0;
	bool Visible = false;
	if(GameClient()->IsTeamPlay())
	{
		TeamDiff = GameClient()->m_Snap.m_aTeamSize[TEAM_RED] - GameClient()->m_Snap.m_aTeamSize[TEAM_BLUE];
		Visible = g_Config.m_ClWarningTeambalance && (TeamDiff >= 2 || TeamDiff <= -2);
	}

	static float s_Phase = 0.0f;
	const float Dt = Client()->RenderFrameTime();
	if(BCUiAnimations::Enabled())
		BCUiAnimations::UpdatePhase(s_Phase, Visible ? 1.0f : 0.0f, Dt, 0.22f);
	else
		s_Phase = Visible ? 1.0f : 0.0f;
	if(s_Phase <= 0.0f)
		return;

	const float Ease = BCUiAnimations::EaseInOutQuad(s_Phase);
	const float Alpha = Ease;
	const float YOff = -8.0f * (1.0f - Ease);

	const char *pText = Localize("Please balance teams!");
	if(Flash)
		SetHudTextColor(1.0f, 1.0f, 0.5f, Alpha);
	else
		SetHudTextColor(0.7f, 0.7f, 0.2f, Alpha);
	TextRender()->Text(5, 50 + YOff, 6, pText, -1.0f);
	SetHudTextColor(TextRender()->DefaultTextColor());
}

void CHud::RenderCursor()
{
	const float Scale = (float)g_Config.m_TcCursorScale / 100.0f;
	if(Scale <= 0.0f)
		return;

	int CurWeapon = 0;
	vec2 TargetPos;
	float Alpha = 1.0f;

	const vec2 Center = GameClient()->m_Camera.m_Center;
	float aPoints[4];
	Graphics()->MapScreenToWorld(Center.x, Center.y, 100.0f, 100.0f, 100.0f, 0, 0, Graphics()->ScreenAspect(), 1.0f, aPoints);
	Graphics()->MapScreen(aPoints[0], aPoints[1], aPoints[2], aPoints[3]);

	if(Client()->State() != IClient::STATE_DEMOPLAYBACK && GameClient()->m_Snap.m_pLocalCharacter)
	{
		// Render local cursor
		CurWeapon = maximum(0, GameClient()->m_Snap.m_pLocalCharacter->m_Weapon % NUM_WEAPONS);
		const int LocalClientId = GameClient()->m_Snap.m_LocalClientId;
		if(GameClient()->m_FastPractice.Enabled() &&
			LocalClientId >= 0 &&
			GameClient()->m_FastPractice.IsPracticeParticipant(LocalClientId))
		{
			CurWeapon = std::clamp(GameClient()->m_aClients[LocalClientId].m_Predicted.m_ActiveWeapon, 0, NUM_WEAPONS - 1);
		}
		TargetPos = GameClient()->m_Controls.m_aTargetPos[g_Config.m_ClDummy];
	}
	else
	{
		// Render spec cursor
		if(!g_Config.m_ClSpecCursor || !GameClient()->m_CursorInfo.IsAvailable())
			return;

		bool RenderSpecCursor = (GameClient()->m_Snap.m_SpecInfo.m_Active && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW) || Client()->State() == IClient::STATE_DEMOPLAYBACK;

		if(!RenderSpecCursor)
			return;

		// Calculate factor to keep cursor on screen
		const vec2 HalfSize = vec2(Center.x - aPoints[0], Center.y - aPoints[1]);
		const vec2 ScreenPos = (GameClient()->m_CursorInfo.WorldTarget() - Center) / GameClient()->m_Camera.m_Zoom;
		const float ClampFactor = maximum(
			1.0f,
			absolute(ScreenPos.x / HalfSize.x),
			absolute(ScreenPos.y / HalfSize.y));

		CurWeapon = maximum(0, GameClient()->m_CursorInfo.Weapon() % NUM_WEAPONS);
		TargetPos = ScreenPos / ClampFactor + Center;
		if(ClampFactor != 1.0f)
			Alpha /= 2.0f;
	}

	SetHudColor(1.0f, 1.0f, 1.0f, Alpha);
	Graphics()->TextureSet(GameClient()->m_GameSkin.m_aSpriteWeaponCursors[CurWeapon]);
	Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_aCursorOffset[CurWeapon], TargetPos.x, TargetPos.y, Scale, Scale);
}

void CHud::PrepareAmmoHealthAndArmorQuads()
{
	float x = 5;
	float y = 5;
	IGraphics::CQuadItem Array[10];

	// ammo of the different weapons
	for(int i = 0; i < NUM_WEAPONS; ++i)
	{
		// 0.6
		for(int n = 0; n < 10; n++)
			Array[n] = IGraphics::CQuadItem(x + n * 12, y, 10, 10);

		m_aAmmoOffset[i] = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

		// 0.7
		if(i == WEAPON_GRENADE)
		{
			// special case for 0.7 grenade
			for(int n = 0; n < 10; n++)
				Array[n] = IGraphics::CQuadItem(1 + x + n * 12, y, 10, 10);
		}
		else
		{
			for(int n = 0; n < 10; n++)
				Array[n] = IGraphics::CQuadItem(x + n * 12, y, 12, 12);
		}

		Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);
	}

	// health
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y, 10, 10);
	m_HealthOffset = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// 0.7
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y, 12, 12);
	Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// empty health
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y, 10, 10);
	m_EmptyHealthOffset = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// 0.7
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y, 12, 12);
	Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// armor meter
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y + 12, 10, 10);
	m_ArmorOffset = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// 0.7
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y + 12, 12, 12);
	Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// empty armor meter
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y + 12, 10, 10);
	m_EmptyArmorOffset = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// 0.7
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y + 12, 12, 12);
	Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);
}

void CHud::RenderAmmoHealthAndArmor(const CNetObj_Character *pCharacter)
{
	if(!pCharacter)
		return;

	bool IsSixupGameSkin = GameClient()->m_GameSkin.IsSixup();
	int QuadOffsetSixup = (IsSixupGameSkin ? 10 : 0);

	if(GameClient()->m_GameInfo.m_HudAmmo)
	{
		// ammo display
		float AmmoOffsetY = GameClient()->m_GameInfo.m_HudHealthArmor ? 24 : 0;
		int CurWeapon = pCharacter->m_Weapon % NUM_WEAPONS;
		// 0.7 only
		if(CurWeapon == WEAPON_NINJA)
		{
			if(!GameClient()->m_GameInfo.m_HudDDRace && Client()->IsSixup())
			{
				const int Max = g_pData->m_Weapons.m_Ninja.m_Duration * Client()->GameTickSpeed() / 1000;
				float NinjaProgress = std::clamp(pCharacter->m_AmmoCount - Client()->GameTick(g_Config.m_ClDummy), 0, Max) / (float)Max;
				RenderNinjaBarPos(5 + 10 * 12, 5, 6.f, 24.f, NinjaProgress);
			}
		}
		else if(CurWeapon >= 0 && GameClient()->m_GameSkin.m_aSpriteWeaponProjectiles[CurWeapon].IsValid())
		{
			Graphics()->TextureSet(GameClient()->m_GameSkin.m_aSpriteWeaponProjectiles[CurWeapon]);
			if(AmmoOffsetY > 0)
			{
				Graphics()->RenderQuadContainerEx(m_HudQuadContainerIndex, m_aAmmoOffset[CurWeapon] + QuadOffsetSixup, std::clamp(pCharacter->m_AmmoCount, 0, 10), 0, AmmoOffsetY);
			}
			else
			{
				Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_aAmmoOffset[CurWeapon] + QuadOffsetSixup, std::clamp(pCharacter->m_AmmoCount, 0, 10));
			}
		}
	}

	if(GameClient()->m_GameInfo.m_HudHealthArmor)
	{
		// health display
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteHealthFull);
		Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_HealthOffset + QuadOffsetSixup, minimum(pCharacter->m_Health, 10));
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteHealthEmpty);
		Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_EmptyHealthOffset + QuadOffsetSixup + minimum(pCharacter->m_Health, 10), 10 - minimum(pCharacter->m_Health, 10));

		// armor display
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteArmorFull);
		Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_ArmorOffset + QuadOffsetSixup, minimum(pCharacter->m_Armor, 10));
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteArmorEmpty);
		Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_ArmorOffset + QuadOffsetSixup + minimum(pCharacter->m_Armor, 10), 10 - minimum(pCharacter->m_Armor, 10));
	}
}

void CHud::PreparePlayerStateQuads()
{
	float x = 5;
	float y = 5 + 24;
	IGraphics::CQuadItem Array[10];

	// Quads for displaying the available and used jumps
	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y, 12, 12);
	m_AirjumpOffset = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	for(int i = 0; i < 10; ++i)
		Array[i] = IGraphics::CQuadItem(x + i * 12, y, 12, 12);
	m_AirjumpEmptyOffset = Graphics()->QuadContainerAddQuads(m_HudQuadContainerIndex, Array, 10);

	// Quads for displaying weapons
	for(int Weapon = 0; Weapon < NUM_WEAPONS; ++Weapon)
	{
		const CDataWeaponspec &WeaponSpec = g_pData->m_Weapons.m_aId[Weapon];
		float ScaleX, ScaleY;
		Graphics()->GetSpriteScale(WeaponSpec.m_pSpriteBody, ScaleX, ScaleY);
		constexpr float HudWeaponScale = 0.25f;
		float Width = WeaponSpec.m_VisualSize * ScaleX * HudWeaponScale;
		float Height = WeaponSpec.m_VisualSize * ScaleY * HudWeaponScale;
		m_aWeaponOffset[Weapon] = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, Width, Height);
	}

	// Quads for displaying capabilities
	m_EndlessJumpOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_EndlessHookOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_JetpackOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_TeleportGrenadeOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_TeleportGunOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_TeleportLaserOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);

	// Quads for displaying prohibited capabilities
	m_SoloOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_CollisionDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_HookHitDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_HammerHitDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_GunHitDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_ShotgunHitDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_GrenadeHitDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_LaserHitDisabledOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);

	// Quads for displaying freeze status
	m_DeepFrozenOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_LiveFrozenOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);

	// Quads for displaying dummy actions
	m_DummyHammerOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_DummyCopyOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);

	// Quads for displaying team modes
	m_PracticeModeOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_LockModeOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
	m_Team0ModeOffset = Graphics()->QuadContainerAddSprite(m_HudQuadContainerIndex, 0.f, 0.f, 12.f, 12.f);
}

void CHud::RenderPlayerState(const int ClientId)
{
	SetHudColor(1.f, 1.f, 1.f, 1.f);

	// pCharacter contains the predicted character for local players or the last snap for players who are spectated
	CCharacterCore *pCharacter = &GameClient()->m_aClients[ClientId].m_Predicted;
	CNetObj_Character *pPlayer = &GameClient()->m_aClients[ClientId].m_RenderCur;
	int TotalJumpsToDisplay = 0;
	if(g_Config.m_ClShowhudJumpsIndicator)
	{
		int AvailableJumpsToDisplay;
		if(GameClient()->m_Snap.m_aCharacters[ClientId].m_HasExtendedDisplayInfo)
		{
			bool Grounded = false;
			if(Collision()->CheckPoint(pPlayer->m_X + CCharacterCore::PhysicalSize() / 2,
				   pPlayer->m_Y + CCharacterCore::PhysicalSize() / 2 + 5))
			{
				Grounded = true;
			}
			if(Collision()->CheckPoint(pPlayer->m_X - CCharacterCore::PhysicalSize() / 2,
				   pPlayer->m_Y + CCharacterCore::PhysicalSize() / 2 + 5))
			{
				Grounded = true;
			}

			int UsedJumps = pCharacter->m_JumpedTotal;
			if(pCharacter->m_Jumps > 1)
			{
				UsedJumps += !Grounded;
			}
			else if(pCharacter->m_Jumps == 1)
			{
				// If the player has only one jump, each jump is the last one
				UsedJumps = pPlayer->m_Jumped & 2;
			}
			else if(pCharacter->m_Jumps == -1)
			{
				// The player has only one ground jump
				UsedJumps = !Grounded;
			}

			if(pCharacter->m_EndlessJump && UsedJumps >= absolute(pCharacter->m_Jumps))
			{
				UsedJumps = absolute(pCharacter->m_Jumps) - 1;
			}

			int UnusedJumps = absolute(pCharacter->m_Jumps) - UsedJumps;
			if(!(pPlayer->m_Jumped & 2) && UnusedJumps <= 0)
			{
				// In some edge cases when the player just got another number of jumps, UnusedJumps is not correct
				UnusedJumps = 1;
			}
			TotalJumpsToDisplay = maximum(minimum(absolute(pCharacter->m_Jumps), 10), 0);
			AvailableJumpsToDisplay = maximum(minimum(UnusedJumps, TotalJumpsToDisplay), 0);
		}
		else
		{
			TotalJumpsToDisplay = AvailableJumpsToDisplay = absolute(GameClient()->m_Snap.m_aCharacters[ClientId].m_ExtendedData.m_Jumps);
		}

		// render available and used jumps
		int JumpsOffsetY = ((GameClient()->m_GameInfo.m_HudHealthArmor && g_Config.m_ClShowhudHealthAmmo ? 24 : 0) +
				    (GameClient()->m_GameInfo.m_HudAmmo && g_Config.m_ClShowhudHealthAmmo ? 12 : 0));
		if(JumpsOffsetY > 0)
		{
			Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudAirjump);
			Graphics()->RenderQuadContainerEx(m_HudQuadContainerIndex, m_AirjumpOffset, AvailableJumpsToDisplay, 0, JumpsOffsetY);
			Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudAirjumpEmpty);
			Graphics()->RenderQuadContainerEx(m_HudQuadContainerIndex, m_AirjumpEmptyOffset + AvailableJumpsToDisplay, TotalJumpsToDisplay - AvailableJumpsToDisplay, 0, JumpsOffsetY);
		}
		else
		{
			Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudAirjump);
			Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_AirjumpOffset, AvailableJumpsToDisplay);
			Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudAirjumpEmpty);
			Graphics()->RenderQuadContainer(m_HudQuadContainerIndex, m_AirjumpEmptyOffset + AvailableJumpsToDisplay, TotalJumpsToDisplay - AvailableJumpsToDisplay);
		}
	}

	float x = 5 + 12;
	float y = (5 + 12 + (GameClient()->m_GameInfo.m_HudHealthArmor && g_Config.m_ClShowhudHealthAmmo ? 24 : 0) +
		   (GameClient()->m_GameInfo.m_HudAmmo && g_Config.m_ClShowhudHealthAmmo ? 12 : 0));

	// render weapons
	{
		constexpr float aWeaponWidth[NUM_WEAPONS] = {16, 12, 12, 12, 12, 12};
		constexpr float aWeaponInitialOffset[NUM_WEAPONS] = {-3, -4, -1, -1, -2, -4};
		int DisplayWeapon = pPlayer->m_Weapon;
		if(GameClient()->m_FastPractice.Enabled() && GameClient()->m_FastPractice.IsPracticeParticipant(ClientId))
			DisplayWeapon = pCharacter->m_ActiveWeapon;
		bool InitialOffsetAdded = false;
		for(int Weapon = 0; Weapon < NUM_WEAPONS; ++Weapon)
		{
			if(!pCharacter->m_aWeapons[Weapon].m_Got)
				continue;
			if(!InitialOffsetAdded)
			{
				x += aWeaponInitialOffset[Weapon];
				InitialOffsetAdded = true;
			}
			if(DisplayWeapon != Weapon)
				SetHudColor(1.0f, 1.0f, 1.0f, 0.4f);
			Graphics()->QuadsSetRotation(pi * 7 / 4);
			Graphics()->TextureSet(GameClient()->m_GameSkin.m_aSpritePickupWeapons[Weapon]);
			Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_aWeaponOffset[Weapon], x, y);
			Graphics()->QuadsSetRotation(0);
			SetHudColor(1.0f, 1.0f, 1.0f, 1.0f);
			x += aWeaponWidth[Weapon];
		}
		if(pCharacter->m_aWeapons[WEAPON_NINJA].m_Got)
		{
			const int Max = g_pData->m_Weapons.m_Ninja.m_Duration * Client()->GameTickSpeed() / 1000;
			float NinjaProgress = std::clamp(pCharacter->m_Ninja.m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Client()->GameTickSpeed() / 1000 - Client()->GameTick(g_Config.m_ClDummy), 0, Max) / (float)Max;
			if(NinjaProgress > 0.0f && GameClient()->m_Snap.m_aCharacters[ClientId].m_HasExtendedDisplayInfo)
			{
				RenderNinjaBarPos(x, y - 12, 6.f, 24.f, NinjaProgress);
			}
		}
	}

	// render capabilities
	x = 5;
	y += 12;
	if(TotalJumpsToDisplay > 0)
	{
		y += 12;
	}
	bool HasCapabilities = false;
	if(pCharacter->m_EndlessJump)
	{
		HasCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudEndlessJump);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_EndlessJumpOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_EndlessHook)
	{
		HasCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudEndlessHook);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_EndlessHookOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_Jetpack)
	{
		HasCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudJetpack);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_JetpackOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_HasTelegunGun && pCharacter->m_aWeapons[WEAPON_GUN].m_Got)
	{
		HasCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudTeleportGun);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_TeleportGunOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_HasTelegunGrenade && pCharacter->m_aWeapons[WEAPON_GRENADE].m_Got)
	{
		HasCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudTeleportGrenade);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_TeleportGrenadeOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_HasTelegunLaser && pCharacter->m_aWeapons[WEAPON_LASER].m_Got)
	{
		HasCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudTeleportLaser);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_TeleportLaserOffset, x, y);
	}

	// render prohibited capabilities
	x = 5;
	if(HasCapabilities)
	{
		y += 12;
	}
	bool HasProhibitedCapabilities = false;
	if(pCharacter->m_Solo)
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudSolo);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_SoloOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_CollisionDisabled)
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudCollisionDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_CollisionDisabledOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_HookHitDisabled)
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudHookHitDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_HookHitDisabledOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_HammerHitDisabled)
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudHammerHitDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_HammerHitDisabledOffset, x, y);
		x += 12;
	}
	if((pCharacter->m_GrenadeHitDisabled && pCharacter->m_HasTelegunGun && pCharacter->m_aWeapons[WEAPON_GUN].m_Got))
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudGunHitDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_LaserHitDisabledOffset, x, y);
		x += 12;
	}
	if((pCharacter->m_ShotgunHitDisabled && pCharacter->m_aWeapons[WEAPON_SHOTGUN].m_Got))
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudShotgunHitDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_ShotgunHitDisabledOffset, x, y);
		x += 12;
	}
	if((pCharacter->m_GrenadeHitDisabled && pCharacter->m_aWeapons[WEAPON_GRENADE].m_Got))
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudGrenadeHitDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_GrenadeHitDisabledOffset, x, y);
		x += 12;
	}
	if((pCharacter->m_LaserHitDisabled && pCharacter->m_aWeapons[WEAPON_LASER].m_Got))
	{
		HasProhibitedCapabilities = true;
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudLaserHitDisabled);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_LaserHitDisabledOffset, x, y);
	}

	// render dummy actions and freeze state
	x = 5;
	if(HasProhibitedCapabilities)
	{
		y += 12;
	}
	if(GameClient()->m_Snap.m_aCharacters[ClientId].m_HasExtendedDisplayInfo && GameClient()->m_Snap.m_aCharacters[ClientId].m_ExtendedData.m_Flags & CHARACTERFLAG_LOCK_MODE)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudLockMode);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_LockModeOffset, x, y);
		x += 12;
	}
	if(GameClient()->m_Snap.m_aCharacters[ClientId].m_HasExtendedDisplayInfo && GameClient()->m_Snap.m_aCharacters[ClientId].m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudPracticeMode);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_PracticeModeOffset, x, y);
		x += 12;
	}
	if(GameClient()->m_Snap.m_aCharacters[ClientId].m_HasExtendedDisplayInfo && GameClient()->m_Snap.m_aCharacters[ClientId].m_ExtendedData.m_Flags & CHARACTERFLAG_TEAM0_MODE)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudTeam0Mode);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_Team0ModeOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_DeepFrozen)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudDeepFrozen);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_DeepFrozenOffset, x, y);
		x += 12;
	}
	if(pCharacter->m_LiveFrozen)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudLiveFrozen);
		Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_LiveFrozenOffset, x, y);
	}
}

void CHud::RenderNinjaBarPos(const float x, float y, const float Width, const float Height, float Progress, const float Alpha)
{
	Progress = std::clamp(Progress, 0.0f, 1.0f);

	// what percentage of the end pieces is used for the progress indicator and how much is the rest
	// half of the ends are used for the progress display
	const float RestPct = 0.5f;
	const float ProgPct = 0.5f;

	const float EndHeight = Width; // to keep the correct scale - the width of the sprite is as long as the height
	const float BarWidth = Width;
	const float WholeBarHeight = Height;
	const float MiddleBarHeight = WholeBarHeight - (EndHeight * 2.0f);
	const float EndProgressHeight = EndHeight * ProgPct;
	const float EndRestHeight = EndHeight * RestPct;
	const float ProgressBarHeight = WholeBarHeight - (EndProgressHeight * 2.0f);
	const float EndProgressProportion = EndProgressHeight / ProgressBarHeight;
	const float MiddleProgressProportion = MiddleBarHeight / ProgressBarHeight;

	// beginning piece
	float BeginningPieceProgress = 1;
	if(Progress <= 1)
	{
		if(Progress <= (EndProgressProportion + MiddleProgressProportion))
		{
			BeginningPieceProgress = 0;
		}
		else
		{
			BeginningPieceProgress = (Progress - EndProgressProportion - MiddleProgressProportion) / EndProgressProportion;
		}
	}
	// empty
	Graphics()->WrapClamp();
	Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudNinjaBarEmptyRight);
	Graphics()->QuadsBegin();
	SetHudColor(1.f, 1.f, 1.f, Alpha);
	// Subset: btm_r, top_r, top_m, btm_m | it is mirrored on the horizontal axe and rotated 90 degrees counterclockwise
	Graphics()->QuadsSetSubsetFree(1, 1, 1, 0, ProgPct - ProgPct * (1.0f - BeginningPieceProgress), 0, ProgPct - ProgPct * (1.0f - BeginningPieceProgress), 1);
	IGraphics::CQuadItem QuadEmptyBeginning(x, y, BarWidth, EndRestHeight + EndProgressHeight * (1.0f - BeginningPieceProgress));
	Graphics()->QuadsDrawTL(&QuadEmptyBeginning, 1);
	Graphics()->QuadsEnd();
	// full
	if(BeginningPieceProgress > 0.0f)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudNinjaBarFullLeft);
		Graphics()->QuadsBegin();
		SetHudColor(1.f, 1.f, 1.f, Alpha);
		// Subset: btm_m, top_m, top_r, btm_r | it is rotated 90 degrees clockwise
		Graphics()->QuadsSetSubsetFree(RestPct + ProgPct * (1.0f - BeginningPieceProgress), 1, RestPct + ProgPct * (1.0f - BeginningPieceProgress), 0, 1, 0, 1, 1);
		IGraphics::CQuadItem QuadFullBeginning(x, y + (EndRestHeight + EndProgressHeight * (1.0f - BeginningPieceProgress)), BarWidth, EndProgressHeight * BeginningPieceProgress);
		Graphics()->QuadsDrawTL(&QuadFullBeginning, 1);
		Graphics()->QuadsEnd();
	}

	// middle piece
	y += EndHeight;

	float MiddlePieceProgress = 1;
	if(Progress <= EndProgressProportion + MiddleProgressProportion)
	{
		if(Progress <= EndProgressProportion)
		{
			MiddlePieceProgress = 0;
		}
		else
		{
			MiddlePieceProgress = (Progress - EndProgressProportion) / MiddleProgressProportion;
		}
	}

	const float FullMiddleBarHeight = MiddleBarHeight * MiddlePieceProgress;
	const float EmptyMiddleBarHeight = MiddleBarHeight - FullMiddleBarHeight;

	// empty ninja bar
	if(EmptyMiddleBarHeight > 0.0f)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudNinjaBarEmpty);
		Graphics()->QuadsBegin();
		SetHudColor(1.f, 1.f, 1.f, Alpha);
		// select the middle portion of the sprite so we don't get edge bleeding
		if(EmptyMiddleBarHeight <= EndHeight)
		{
			// prevent pixel puree, select only a small slice
			// Subset: btm_r, top_r, top_m, btm_m | it is mirrored on the horizontal axe and rotated 90 degrees counterclockwise
			Graphics()->QuadsSetSubsetFree(1, 1, 1, 0, 1.0f - (EmptyMiddleBarHeight / EndHeight), 0, 1.0f - (EmptyMiddleBarHeight / EndHeight), 1);
		}
		else
		{
			// Subset: btm_r, top_r, top_l, btm_l | it is mirrored on the horizontal axe and rotated 90 degrees counterclockwise
			Graphics()->QuadsSetSubsetFree(1, 1, 1, 0, 0, 0, 0, 1);
		}
		IGraphics::CQuadItem QuadEmpty(x, y, BarWidth, EmptyMiddleBarHeight);
		Graphics()->QuadsDrawTL(&QuadEmpty, 1);
		Graphics()->QuadsEnd();
	}

	// full ninja bar
	Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudNinjaBarFull);
	Graphics()->QuadsBegin();
	SetHudColor(1.f, 1.f, 1.f, Alpha);
	// select the middle portion of the sprite so we don't get edge bleeding
	if(FullMiddleBarHeight <= EndHeight)
	{
		// prevent pixel puree, select only a small slice
		// Subset: btm_m, top_m, top_r, btm_r | it is rotated 90 degrees clockwise
		Graphics()->QuadsSetSubsetFree(1.0f - (FullMiddleBarHeight / EndHeight), 1, 1.0f - (FullMiddleBarHeight / EndHeight), 0, 1, 0, 1, 1);
	}
	else
	{
		// Subset: btm_l, top_l, top_r, btm_r | it is rotated 90 degrees clockwise
		Graphics()->QuadsSetSubsetFree(0, 1, 0, 0, 1, 0, 1, 1);
	}
	IGraphics::CQuadItem QuadFull(x, y + EmptyMiddleBarHeight, BarWidth, FullMiddleBarHeight);
	Graphics()->QuadsDrawTL(&QuadFull, 1);
	Graphics()->QuadsEnd();

	// ending piece
	y += MiddleBarHeight;
	float EndingPieceProgress = 1;
	if(Progress <= EndProgressProportion)
	{
		EndingPieceProgress = Progress / EndProgressProportion;
	}
	// empty
	if(EndingPieceProgress < 1.0f)
	{
		Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudNinjaBarEmptyRight);
		Graphics()->QuadsBegin();
		SetHudColor(1.f, 1.f, 1.f, Alpha);
		// Subset: btm_l, top_l, top_m, btm_m | it is rotated 90 degrees clockwise
		Graphics()->QuadsSetSubsetFree(0, 1, 0, 0, ProgPct - ProgPct * EndingPieceProgress, 0, ProgPct - ProgPct * EndingPieceProgress, 1);
		IGraphics::CQuadItem QuadEmptyEnding(x, y, BarWidth, EndProgressHeight * (1.0f - EndingPieceProgress));
		Graphics()->QuadsDrawTL(&QuadEmptyEnding, 1);
		Graphics()->QuadsEnd();
	}
	// full
	Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudNinjaBarFullLeft);
	Graphics()->QuadsBegin();
	SetHudColor(1.f, 1.f, 1.f, Alpha);
	// Subset: btm_m, top_m, top_l, btm_l | it is mirrored on the horizontal axe and rotated 90 degrees counterclockwise
	Graphics()->QuadsSetSubsetFree(RestPct + ProgPct * EndingPieceProgress, 1, RestPct + ProgPct * EndingPieceProgress, 0, 0, 0, 0, 1);
	IGraphics::CQuadItem QuadFullEnding(x, y + (EndProgressHeight * (1.0f - EndingPieceProgress)), BarWidth, EndRestHeight + EndProgressHeight * EndingPieceProgress);
	Graphics()->QuadsDrawTL(&QuadFullEnding, 1);
	Graphics()->QuadsEnd();

	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	SetHudColor(1.f, 1.f, 1.f, 1.f);
	Graphics()->WrapNormal();
}

void CHud::RenderSpectatorCount(bool ForcePreview)
{
	const auto Layout = HudLayout::Get(HudLayout::MODULE_SPECTATOR_COUNT, m_Width, m_Height);
	if(!ForcePreview && !g_Config.m_ClShowhudSpectatorCount)
		return;

	const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
	const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
	const unsigned BackgroundColor = Layout.m_BackgroundColor;

	int aSpectatorIds[MAX_CLIENTS];
	bool aSpectatorAdded[MAX_CLIENTS] = {};
	int NumSpectatorIds = 0;
	bool HasExactSpectatorNames = false;
	bool HasReliableFallbackSpectatorNames = false;
	auto AddSpectator = [&](int ClientId) {
		if(ClientId < 0 || ClientId >= MAX_CLIENTS || aSpectatorAdded[ClientId])
			return;
		aSpectatorAdded[ClientId] = true;
		aSpectatorIds[NumSpectatorIds] = ClientId;
		++NumSpectatorIds;
	};

	int Count = 0;
	if(ForcePreview)
	{
		Count = 3;
		HasExactSpectatorNames = true;
	}
	else
	{
		const int LocalId = GameClient()->m_aLocalIds[0];
		const int DummyId = Client()->DummyConnected() ? GameClient()->m_aLocalIds[1] : -1;
		if(Client()->IsSixup())
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(i == LocalId || i == DummyId)
					continue;

				if(Client()->m_TranslationContext.m_aClients[i].m_PlayerFlags7 & protocol7::PLAYERFLAG_WATCHING)
					AddSpectator(i);
			}
			Count = NumSpectatorIds;
			HasExactSpectatorNames = true;
		}
		else
		{
			const CNetObj_SpectatorCount *pSpectatorCount = GameClient()->m_Snap.m_pSpectatorCount;
			if(!pSpectatorCount)
			{
				m_LastSpectatorCountTick = Client()->GameTick(g_Config.m_ClDummy);
				return;
			}
			Count = pSpectatorCount->m_NumSpectators;

			if(GameClient()->m_Snap.m_NumSpectatorWatchers > 0)
			{
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(GameClient()->m_Snap.m_aSpectatorWatchers[i])
						AddSpectator(i);
				}
				HasExactSpectatorNames = true;
			}
			else
			{
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(i == LocalId || i == DummyId)
						continue;

					const CNetObj_PlayerInfo *pInfo = GameClient()->m_Snap.m_apPlayerInfos[i];
					if(!pInfo || !GameClient()->m_aClients[i].m_Active)
						continue;

					const bool IsLikelySpectator = pInfo->m_Team == TEAM_SPECTATORS || GameClient()->m_aClients[i].m_Spec || GameClient()->m_aClients[i].m_Paused;
					if(IsLikelySpectator)
						AddSpectator(i);
				}
				HasReliableFallbackSpectatorNames = NumSpectatorIds == Count;
			}
		}

		if(Count == 0)
		{
			m_LastSpectatorCountTick = Client()->GameTick(g_Config.m_ClDummy);
			return;
		}

		if(Client()->GameTick(g_Config.m_ClDummy) < m_LastSpectatorCountTick + Client()->GameTickSpeed())
			return;
	}

	char aCountBuf[16];
	str_format(aCountBuf, sizeof(aCountBuf), "%d", Count);

	constexpr int MAX_NAME_LINES = 6;
	char aaNameLines[MAX_NAME_LINES][MAX_NAME_LENGTH + 8] = {};
	int NumNameLines = 0;
	if(ForcePreview)
	{
		str_copy(aaNameLines[0], "watcher_one");
		str_copy(aaNameLines[1], "watcher_two");
		str_copy(aaNameLines[2], "+1");
		NumNameLines = 3;
	}
	else if((HasExactSpectatorNames || HasReliableFallbackSpectatorNames) && NumSpectatorIds > 0)
	{
		const int MaxVisibleNames = MAX_NAME_LINES - 1;
		int VisibleNames = minimum(NumSpectatorIds, MaxVisibleNames);

		int ShownNames = 0;
		for(int i = 0; i < NumSpectatorIds && ShownNames < VisibleNames; i++)
		{
			const char *pName = GameClient()->m_aClients[aSpectatorIds[i]].m_aName;
			if(pName[0] == '\0')
				continue;
			str_copy(aaNameLines[NumNameLines], pName);
			++NumNameLines;
			++ShownNames;
		}

		const int Remaining = maximum(0, Count - ShownNames);
		if(Remaining > 0 && NumNameLines < MAX_NAME_LINES)
		{
			str_format(aaNameLines[NumNameLines], sizeof(aaNameLines[NumNameLines]), "+%d", Remaining);
			++NumNameLines;
		}
	}

	const float Fontsize = 6.0f * Scale;
	const float LineHeight = Fontsize + 2.0f * Scale;
	const float PaddingX = 2.0f * Scale;
	const float PaddingY = 2.0f * Scale;
	const float HeaderWidth = Fontsize + 3.0f * Scale + TextRender()->TextWidth(Fontsize, aCountBuf);
	float MaxLineWidth = HeaderWidth;
	for(int i = 0; i < NumNameLines; i++)
		MaxLineWidth = maximum(MaxLineWidth, TextRender()->TextWidth(Fontsize, aaNameLines[i]));

	const float BoxHeight = PaddingY * 2.0f + LineHeight * (1 + NumNameLines);
	const float BoxWidth = PaddingX * 2.0f + MaxLineWidth;
	float StartX = m_Width - BoxWidth;
	float StartY = 285.0f - BoxHeight - 4.0f * Scale;
	if(g_Config.m_ClShowhudPlayerPosition || g_Config.m_ClShowhudPlayerSpeed || g_Config.m_ClShowhudPlayerAngle)
		StartY -= 4.0f * Scale;
	StartY -= GetMovementInformationBoxHeight(ForcePreview);
	if(g_Config.m_ClShowhudScore)
		StartY -= 56.0f;
	const bool DummyActionsVisible = g_Config.m_ClShowhudDummyActions && Client()->DummyConnected() && GameClient()->m_Snap.m_pGameInfoObj && !(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER);
	if(DummyActionsVisible)
		StartY -= 29.0f + 4.0f * Scale;
	StartX = std::clamp(StartX, 0.0f, maximum(0.0f, m_Width - BoxWidth));
	StartY = std::clamp(StartY, 0.0f, maximum(0.0f, m_Height - BoxHeight));

	if(BackgroundEnabled)
	{
		const int Corners = HudModuleBackgroundCorners(IGraphics::CORNER_L, StartX, StartY, BoxWidth, BoxHeight, m_Width, m_Height);
		Graphics()->DrawRect(StartX, StartY, BoxWidth, BoxHeight, ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)), Corners, 5.0f * Scale);
	}

	float y = StartY + PaddingY;
	float x = StartX + PaddingX;

	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->Text(x, y, Fontsize, FontIcons::FONT_ICON_EYE, -1.0f);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->Text(x + Fontsize + 3.0f * Scale, y, Fontsize, aCountBuf, -1.0f);

	y += LineHeight;
	for(int i = 0; i < NumNameLines; i++)
	{
		TextRender()->Text(x, y, Fontsize, aaNameLines[i], -1.0f);
		y += LineHeight;
	}
}

void CHud::RenderDummyActions()
{
	if(!g_Config.m_ClShowhudDummyActions || (GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER) || !Client()->DummyConnected())
	{
		return;
	}
	// render small dummy actions hud
	const float BoxHeight = 29.0f;
	const float BoxWidth = 16.0f;

	float StartX = m_Width - BoxWidth;
	float StartY = 285.0f - BoxHeight - 4; // 4 units distance to the next display;
	if(g_Config.m_ClShowhudPlayerPosition || g_Config.m_ClShowhudPlayerSpeed || g_Config.m_ClShowhudPlayerAngle)
	{
		StartY -= 4;
	}
	StartY -= GetMovementInformationBoxHeight();

	if(g_Config.m_ClShowhudScore)
	{
		StartY -= 56;
	}

	Graphics()->DrawRect(StartX, StartY, BoxWidth, BoxHeight, ApplyHudAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.4f)), IGraphics::CORNER_L, 5.0f);

	float y = StartY + 2;
	float x = StartX + 2;
	SetHudColor(1.0f, 1.0f, 1.0f, 0.4f);
	if(g_Config.m_ClDummyHammer)
	{
		SetHudColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudDummyHammer);
	Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_DummyHammerOffset, x, y);
	y += 13;
	SetHudColor(1.0f, 1.0f, 1.0f, 0.4f);
	if(g_Config.m_ClDummyCopyMoves)
	{
		SetHudColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	Graphics()->TextureSet(GameClient()->m_HudSkin.m_SpriteHudDummyCopy);
	Graphics()->RenderQuadContainerAsSprite(m_HudQuadContainerIndex, m_DummyCopyOffset, x, y);
}

inline int CHud::GetDigitsIndex(int Value, int Max)
{
	if(Value < 0)
	{
		Value *= -1;
	}
	int DigitsIndex = std::log10((Value ? Value : 1));
	if(DigitsIndex > Max)
	{
		DigitsIndex = Max;
	}
	if(DigitsIndex < 0)
	{
		DigitsIndex = 0;
	}
	return DigitsIndex;
}

inline float CHud::GetMovementInformationBoxHeight(bool ForcePreview)
{
	const bool ForceFullPreview = ForcePreview && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK;
	if(ForceFullPreview)
		return 3.0f * MOVEMENT_INFORMATION_LINE_HEIGHT * 2.0f + 2.0f * MOVEMENT_INFORMATION_LINE_HEIGHT + MOVEMENT_INFORMATION_LINE_HEIGHT + 2.0f;

	const int ClientId = GameClient()->m_Snap.m_SpecInfo.m_Active ? GameClient()->m_Snap.m_SpecInfo.m_SpectatorId : GameClient()->m_Snap.m_LocalClientId;
	const bool ShowDummyCoordIndicator = CanRenderCoordIndicator(ClientId, ForcePreview);
	if(GameClient()->m_Snap.m_SpecInfo.m_Active && (GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW || GameClient()->m_aClients[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId].m_SpecCharPresent))
	{
		float BoxHeight = g_Config.m_ClShowhudPlayerPosition ? 3.0f * MOVEMENT_INFORMATION_LINE_HEIGHT + 2.0f : 0.0f;
		if(ShowDummyCoordIndicator)
			BoxHeight += MOVEMENT_INFORMATION_LINE_HEIGHT;
		return BoxHeight;
	}
	float BoxHeight = 3.0f * MOVEMENT_INFORMATION_LINE_HEIGHT * (g_Config.m_ClShowhudPlayerPosition + g_Config.m_ClShowhudPlayerSpeed) + 2.0f * MOVEMENT_INFORMATION_LINE_HEIGHT * g_Config.m_ClShowhudPlayerAngle;
	if(ShowDummyCoordIndicator)
		BoxHeight += MOVEMENT_INFORMATION_LINE_HEIGHT;
	if(g_Config.m_ClShowhudPlayerPosition || g_Config.m_ClShowhudPlayerSpeed || g_Config.m_ClShowhudPlayerAngle)
	{
		BoxHeight += 2.0f;
	}
	return BoxHeight;
}

bool CHud::CanRenderCoordIndicator(int ClientId, bool ForcePreview) const
{
	if(!g_Config.m_BcShowhudDummyCoordIndicator)
		return false;
	if(ForcePreview)
		return true;
	return ClientId >= 0 && ClientId < MAX_CLIENTS;
}

bool CHud::HasPlayerBelowOnSameX(int ClientId, const CMovementInformation &Info) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return false;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == ClientId || !GameClient()->m_aClients[i].m_Active || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;

		const CNetObj_PlayerInfo *pInfo = GameClient()->m_Snap.m_apPlayerInfos[i];
		if(!pInfo || pInfo->m_Team == TEAM_SPECTATORS)
			continue;

		const CMovementInformation OtherInfo = GetMovementInformation(i, i == GameClient()->m_aLocalIds[1] ? IClient::CONN_DUMMY : IClient::CONN_MAIN);
		if(std::fabs(Info.m_Pos.x - OtherInfo.m_Pos.x) < 0.01f && OtherInfo.m_Pos.y > Info.m_Pos.y)
			return true;
	}

	return false;
}

void CHud::UpdateMovementInformationTextContainer(STextContainerIndex &TextContainer, float FontSize, float Value, float &PrevValue)
{
	Value = std::round(Value * 100.0f) / 100.0f; // Round to 2dp
	if(TextContainer.Valid() && PrevValue == Value)
		return;
	PrevValue = Value;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%.2f", Value);

	CTextCursor Cursor;
	Cursor.m_FontSize = FontSize;
	TextRender()->RecreateTextContainer(TextContainer, &Cursor, aBuf);
}

void CHud::RenderMovementInformationTextContainer(STextContainerIndex &TextContainer, const ColorRGBA &Color, float X, float Y)
{
	if(TextContainer.Valid())
	{
		TextRender()->RenderTextContainer(TextContainer, ApplyHudAlpha(Color), ApplyHudAlpha(TextRender()->DefaultTextOutlineColor()), X - TextRender()->GetBoundingBoxTextContainer(TextContainer).m_W, Y);
	}
}

CHud::CMovementInformation CHud::GetMovementInformation(int ClientId, int Conn) const
{
	CMovementInformation Out;
	if(ClientId == SPEC_FREEVIEW)
	{
		Out.m_Pos = GameClient()->m_Camera.m_Center / 32.0f;
	}
	else if(GameClient()->m_aClients[ClientId].m_SpecCharPresent)
	{
		Out.m_Pos = GameClient()->m_aClients[ClientId].m_SpecChar / 32.0f;
	}
	else
	{
		const CNetObj_Character *pPrevChar = &GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev;
		const CNetObj_Character *pCurChar = &GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur;
		const float IntraTick = Client()->IntraGameTick(Conn);

		// To make the player position relative to blocks we need to divide by the block size
		Out.m_Pos = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pCurChar->m_X, pCurChar->m_Y), IntraTick) / 32.0f;

		const vec2 Vel = mix(vec2(pPrevChar->m_VelX, pPrevChar->m_VelY), vec2(pCurChar->m_VelX, pCurChar->m_VelY), IntraTick);

		float VelspeedX = Vel.x / 256.0f * Client()->GameTickSpeed();
		if(Vel.x >= -1.0f && Vel.x <= 1.0f)
		{
			VelspeedX = 0.0f;
		}
		float VelspeedY = Vel.y / 256.0f * Client()->GameTickSpeed();
		if(Vel.y >= -128.0f && Vel.y <= 128.0f)
		{
			VelspeedY = 0.0f;
		}
		// We show the speed in Blocks per Second (Bps) and therefore have to divide by the block size
		Out.m_Speed.x = VelspeedX / 32.0f;
		float VelspeedLength = length(vec2(Vel.x, Vel.y) / 256.0f) * Client()->GameTickSpeed();
		// Todo: Use Velramp tuning of each individual player
		// Since these tuning parameters are almost never changed, the default values are sufficient in most cases
		float Ramp = VelocityRamp(VelspeedLength, GameClient()->m_aTuning[Conn].m_VelrampStart, GameClient()->m_aTuning[Conn].m_VelrampRange, GameClient()->m_aTuning[Conn].m_VelrampCurvature);
		Out.m_Speed.x *= Ramp;
		Out.m_Speed.y = VelspeedY / 32.0f;

		float Angle = GameClient()->m_Players.GetPlayerTargetAngle(pPrevChar, pCurChar, ClientId, IntraTick);
		if(Angle < 0.0f)
		{
			Angle += 2.0f * pi;
		}
		Out.m_Angle = Angle * 180.0f / pi;
	}
	return Out;
}

void CHud::RenderMovementInformation(bool ForcePreview)
{
	const auto Layout = HudLayout::Get(HudLayout::MODULE_MOVEMENT_INFO, m_Width, m_Height);

	const int ClientId = GameClient()->m_Snap.m_SpecInfo.m_Active ? GameClient()->m_Snap.m_SpecInfo.m_SpectatorId : GameClient()->m_Snap.m_LocalClientId;
	const bool HasValidClientId = ClientId == SPEC_FREEVIEW || (ClientId >= 0 && ClientId < MAX_CLIENTS);
	if(!HasValidClientId && !ForcePreview)
		return;
	const bool PosOnly = HasValidClientId && (ClientId == SPEC_FREEVIEW || (ClientId >= 0 && ClientId < MAX_CLIENTS && GameClient()->m_aClients[ClientId].m_SpecCharPresent));
	bool HasDummyInfo = false;
	CMovementInformation DummyInfo{};

	if(Client()->DummyConnected())
	{
		int DummyClientId = -1;

		if(GameClient()->m_Snap.m_SpecInfo.m_Active)
		{
			const int SpectId = GameClient()->m_Snap.m_SpecInfo.m_SpectatorId;

			if(SpectId == GameClient()->m_aLocalIds[0])
			{
				DummyClientId = GameClient()->m_aLocalIds[1];
			}
			else if(SpectId == GameClient()->m_aLocalIds[1])
			{
				DummyClientId = GameClient()->m_aLocalIds[0];
			}
			else
			{
				DummyClientId = GameClient()->m_aLocalIds[1 - (g_Config.m_ClDummy ? 1 : 0)];
			}
		}
		else
		{
			DummyClientId = GameClient()->m_aLocalIds[1 - (g_Config.m_ClDummy ? 1 : 0)];
		}

		if(DummyClientId >= 0 && DummyClientId < MAX_CLIENTS &&
			GameClient()->m_aClients[DummyClientId].m_Active)
		{
			DummyInfo = GetMovementInformation(
				DummyClientId,
				DummyClientId == GameClient()->m_aLocalIds[1]);
			HasDummyInfo = true;
		}
	}
	else if(ForcePreview)
	{
		HasDummyInfo = true;
		DummyInfo.m_Pos = vec2(10.50f, 60.53f);
		DummyInfo.m_Speed = vec2(0.00f, 0.00f);
		DummyInfo.m_Angle = 25.36f;
	}

	const bool ForceFullPreview = ForcePreview && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK;
	const bool ShowDummyPos = HasDummyInfo && (ForceFullPreview || (g_Config.m_ClShowhudPlayerPosition && g_Config.m_TcShowhudDummyPosition));
	const bool ShowDummySpeed = HasDummyInfo && !PosOnly && (ForceFullPreview || (g_Config.m_ClShowhudPlayerSpeed && g_Config.m_TcShowhudDummySpeed));
	const bool ShowDummyAngle = HasDummyInfo && !PosOnly && (ForceFullPreview || (g_Config.m_ClShowhudPlayerAngle && g_Config.m_TcShowhudDummyAngle));
	const bool ShowDummyCoordIndicator = ForceFullPreview || CanRenderCoordIndicator(ClientId, ForcePreview);
	const bool ShowPlayerPos = ForceFullPreview || g_Config.m_ClShowhudPlayerPosition;
	const bool ShowPlayerSpeed = !PosOnly && (ForceFullPreview || g_Config.m_ClShowhudPlayerSpeed);
	const bool ShowPlayerAngle = !PosOnly && (ForceFullPreview || g_Config.m_ClShowhudPlayerAngle);

	// Draw the information depending on settings: Position, speed and target angle
	// This display is only to present the available information from the last snapshot, not to interpolate or predict
	if(!ShowPlayerPos && !ShowPlayerSpeed && !ShowPlayerAngle && !ShowDummyCoordIndicator)
	{
		return;
	}

	const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
	const float LineSpacer = 1.0f * Scale; // above and below each entry
	const float Fontsize = 6.0f * Scale;
	const float LineHeight = MOVEMENT_INFORMATION_LINE_HEIGHT * Scale;

	float BoxHeight = GetMovementInformationBoxHeight(ForcePreview) * Scale;

	if(ShowDummyPos)
		BoxHeight += 2.0f * LineHeight;
	if(ShowDummySpeed)
		BoxHeight += 2.0f * LineHeight;
	if(ShowDummyAngle)
		BoxHeight += 1.0f * LineHeight;

	const float BoxWidth = 62.0f * Scale;

	float StartX = m_Width - BoxWidth;
	float StartY = 285.0f - BoxHeight - 4.0f * Scale;
	if(g_Config.m_ClShowhudScore)
		StartY -= 56.0f;
	StartX = std::clamp(StartX, 0.0f, maximum(0.0f, m_Width - BoxWidth));
	StartY = std::clamp(StartY, 0.0f, maximum(0.0f, m_Height - BoxHeight));
	const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
	const unsigned BackgroundColor = Layout.m_BackgroundColor;

	if(BackgroundEnabled)
	{
		const int Corners = HudModuleBackgroundCorners(IGraphics::CORNER_L, StartX, StartY, BoxWidth, BoxHeight, m_Width, m_Height);
		Graphics()->DrawRect(StartX, StartY, BoxWidth, BoxHeight, ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)), Corners, 5.0f);
	}

	CMovementInformation Info{};
	if(HasValidClientId && ClientId != SPEC_FREEVIEW)
		Info = GetMovementInformation(ClientId, g_Config.m_ClDummy);
	else if(HasValidClientId && ClientId == SPEC_FREEVIEW)
		Info = GetMovementInformation(ClientId, g_Config.m_ClDummy);
	else
	{
		Info.m_Pos = vec2(12.50f, 60.53f);
		Info.m_Speed = vec2(0.00f, 0.00f);
		Info.m_Angle = 25.36f;
	}

	const bool HasPlayerBelow = !ForcePreview && HasValidClientId && ClientId != SPEC_FREEVIEW && HasPlayerBelowOnSameX(ClientId, Info);

	float y = StartY + LineSpacer * 2.0f;
	const float LeftX = StartX + 2.0f;
	const float RightX = StartX + BoxWidth - 2.0f;

	if(ShowPlayerPos)
	{
		TextRender()->Text(LeftX, y, Fontsize, Localize("Position:"), -1.0f);
		y += LineHeight;

		TextRender()->Text(LeftX, y, Fontsize, "X:", -1.0f);
		UpdateMovementInformationTextContainer(m_aPlayerPositionContainers[0], Fontsize, Info.m_Pos.x, m_aPlayerPrevPosition[0]);

		ColorRGBA TextColor = TextRender()->DefaultTextColor();
		if(ShowDummyPos && fabsf(Info.m_Pos.x - DummyInfo.m_Pos.x) < 0.01f)
			TextColor = ColorRGBA(0.2f, 1.0f, 0.2f, 1.0f);

		RenderMovementInformationTextContainer(m_aPlayerPositionContainers[0], TextColor, RightX, y);
		y += LineHeight;

		TextRender()->Text(LeftX, y, Fontsize, "Y:", -1.0f);
		UpdateMovementInformationTextContainer(m_aPlayerPositionContainers[1], Fontsize, Info.m_Pos.y, m_aPlayerPrevPosition[1]);
		RenderMovementInformationTextContainer(m_aPlayerPositionContainers[1], TextRender()->DefaultTextColor(), RightX, y);
		y += LineHeight;

		if(ShowDummyPos)
		{
			char aBuf[32];

			TextRender()->Text(LeftX, y, Fontsize, "DX:", -1.0f);
			str_format(aBuf, sizeof(aBuf), "%.2f", DummyInfo.m_Pos.x);

			ColorRGBA DummyTextColor = TextRender()->DefaultTextColor();
			if(fabsf(Info.m_Pos.x - DummyInfo.m_Pos.x) < 0.01f)
				DummyTextColor = ColorRGBA(0.2f, 1.0f, 0.2f, 1.0f);

			SetHudTextColor(DummyTextColor);
			TextRender()->Text(RightX - TextRender()->TextWidth(Fontsize, aBuf), y, Fontsize, aBuf, -1.0f);
			SetHudTextColor(1.0f, 1.0f, 1.0f, 1.0f);
			y += LineHeight;

			TextRender()->Text(LeftX, y, Fontsize, "DY:", -1.0f);
			str_format(aBuf, sizeof(aBuf), "%.2f", DummyInfo.m_Pos.y);
			TextRender()->Text(RightX - TextRender()->TextWidth(Fontsize, aBuf), y, Fontsize, aBuf, -1.0f);
			y += LineHeight;
		}
	}

	if(ShowPlayerSpeed)
	{
		TextRender()->Text(LeftX, y, Fontsize, Localize("Speed:"), -1.0f);
		y += LineHeight;

		const char aaCoordinates[][4] = {"X:", "Y:"};
		for(int i = 0; i < 2; i++)
		{
			ColorRGBA Color(1.0f, 1.0f, 1.0f, 1.0f);
			if(m_aLastPlayerSpeedChange[i] == ESpeedChange::INCREASE)
				Color = ColorRGBA(0.0f, 1.0f, 0.0f, 1.0f);
			if(m_aLastPlayerSpeedChange[i] == ESpeedChange::DECREASE)
				Color = ColorRGBA(1.0f, 0.5f, 0.5f, 1.0f);
			TextRender()->Text(LeftX, y, Fontsize, aaCoordinates[i], -1.0f);
			UpdateMovementInformationTextContainer(m_aPlayerSpeedTextContainers[i], Fontsize, i == 0 ? Info.m_Speed.x : Info.m_Speed.y, m_aPlayerPrevSpeed[i]);
			RenderMovementInformationTextContainer(m_aPlayerSpeedTextContainers[i], Color, RightX, y);
			y += LineHeight;
		}

		if(ShowDummySpeed)
		{
			char aBuf[32];

			TextRender()->Text(LeftX, y, Fontsize, "DX:", -1.0f);
			str_format(aBuf, sizeof(aBuf), "%.2f", DummyInfo.m_Speed.x);
			TextRender()->Text(RightX - TextRender()->TextWidth(Fontsize, aBuf), y, Fontsize, aBuf, -1.0f);
			y += LineHeight;

			TextRender()->Text(LeftX, y, Fontsize, "DY:", -1.0f);
			str_format(aBuf, sizeof(aBuf), "%.2f", DummyInfo.m_Speed.y);
			TextRender()->Text(RightX - TextRender()->TextWidth(Fontsize, aBuf), y, Fontsize, aBuf, -1.0f);
			y += LineHeight;
		}

		SetHudTextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if(ShowPlayerAngle)
	{
		TextRender()->Text(LeftX, y, Fontsize, Localize("Angle:"), -1.0f);
		y += LineHeight;

		UpdateMovementInformationTextContainer(m_PlayerAngleTextContainerIndex, Fontsize, Info.m_Angle, m_PlayerPrevAngle);
		RenderMovementInformationTextContainer(m_PlayerAngleTextContainerIndex, TextRender()->DefaultTextColor(), RightX, y);
		y += LineHeight;

		if(ShowDummyAngle)
		{
			char aBuf[32];

			TextRender()->Text(LeftX, y, Fontsize, "DA:", -1.0f);
			str_format(aBuf, sizeof(aBuf), "%.2f", DummyInfo.m_Angle);
			TextRender()->Text(RightX - TextRender()->TextWidth(Fontsize, aBuf), y, Fontsize, aBuf, -1.0f);
		}
	}

	if(ShowDummyCoordIndicator)
	{
		const ColorRGBA IndicatorNormalColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BcShowhudDummyCoordIndicatorColor));
		const ColorRGBA IndicatorSameHeightColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BcShowhudDummyCoordIndicatorSameHeightColor));
		const ColorRGBA IndicatorColor = (ForcePreview || HasPlayerBelow) ? IndicatorSameHeightColor : IndicatorNormalColor;

		TextRender()->Text(LeftX, y, Fontsize, Localize("Player below"), -1.0f);

		const float CircleX = RightX - 3.0f * Scale;
		const float CircleY = y + 3.0f * Scale;
		ColorRGBA GlowColor = IndicatorColor;
		GlowColor.a *= 0.35f;

		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(ApplyHudAlpha(GlowColor));
		Graphics()->DrawCircle(CircleX, CircleY, 3.0f * Scale, 16);
		Graphics()->SetColor(ApplyHudAlpha(IndicatorColor));
		Graphics()->DrawCircle(CircleX, CircleY, 1.8f * Scale, 16);
		Graphics()->QuadsEnd();
	}
}

void CHud::RenderSpectatorHud()
{
	if(!g_Config.m_ClShowhudSpectator)
		return;

	// BestClient
	float AdjustedHeight = m_Height - (g_Config.m_TcStatusBar ? g_Config.m_TcStatusBarHeight : 0.0f);
	// draw the box
	Graphics()->DrawRect(m_Width - 180.0f, AdjustedHeight - 15.0f, 180.0f, 15.0f, ApplyHudAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.4f)), IGraphics::CORNER_TL, 5.0f);

	// draw the text
	char aBuf[128];
	if(GameClient()->m_MultiViewActivated)
	{
		str_copy(aBuf, Localize("Multi-View"));
	}
	else if(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW)
	{
		const auto &Player = GameClient()->m_aClients[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId];
		if(g_Config.m_ClShowIds)
			str_format(aBuf, sizeof(aBuf), Localize("Following %d: %s", "Spectating"), Player.ClientId(), Player.m_aName);
		else
			str_format(aBuf, sizeof(aBuf), Localize("Following %s", "Spectating"), Player.m_aName);
	}
	else
	{
		str_copy(aBuf, Localize("Free-View"));
	}
	TextRender()->Text(m_Width - 174.0f, AdjustedHeight - 15.0f + (15.f - 8.f) / 2.f, 8.0f, aBuf, -1.0f);

	// draw the camera info
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK && GameClient()->m_Camera.SpectatingPlayer() && GameClient()->m_Camera.CanUseAutoSpecCamera() && g_Config.m_ClSpecAutoSync)
	{
		bool AutoSpecCameraEnabled = GameClient()->m_Camera.m_AutoSpecCamera;
		const char *pLabelText = Localize("AUTO", "Spectating Camera Mode Icon");
		const float TextWidth = TextRender()->TextWidth(6.0f, pLabelText);

		constexpr float RightMargin = 4.0f;
		constexpr float IconWidth = 6.0f;
		constexpr float Padding = 3.0f;
		const float TagWidth = IconWidth + TextWidth + Padding * 3.0f;
		const float TagX = m_Width - RightMargin - TagWidth;
		Graphics()->DrawRect(TagX, m_Height - 12.0f, TagWidth, 10.0f, ApplyHudAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, AutoSpecCameraEnabled ? 0.50f : 0.10f)), IGraphics::CORNER_ALL, 2.5f);
		SetHudTextColor(1, 1, 1, AutoSpecCameraEnabled ? 1.0f : 0.65f);
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->Text(TagX + Padding, m_Height - 10.0f, 6.0f, FontIcons::FONT_ICON_CAMERA, -1.0f);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		TextRender()->Text(TagX + Padding + IconWidth + Padding, m_Height - 10.0f, 6.0f, pLabelText, -1.0f);
		SetHudTextColor(1, 1, 1, 1);
	}
}

void CHud::RenderLocalTime(float x, bool ForcePreview)
{
	const auto Layout = HudLayout::Get(HudLayout::MODULE_LOCAL_TIME, m_Width, m_Height);
	if(!ForcePreview && !g_Config.m_ClShowLocalTimeAlways && !GameClient()->m_Scoreboard.IsActive())
		return;

	const bool Seconds = g_Config.m_TcShowLocalTimeSeconds; // BestClient
	const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
	const bool BackgroundEnabled = Layout.m_BackgroundEnabled;
	const unsigned BackgroundColor = Layout.m_BackgroundColor;

	char aTimeStr[16];
	str_timestamp_format(aTimeStr, sizeof(aTimeStr), Seconds ? "%H:%M.%S" : "%H:%M");
	const float FontSize = 5.0f * Scale;
	const float Width = std::round(TextRender()->TextBoundingBox(FontSize, aTimeStr).m_W);
	const float BoxWidth = Width + 10.0f * Scale;
	const float BoxHeight = 12.5f * Scale;
	float DrawX = x - (Width + 15.0f * Scale);
	float DrawY = 0.0f;
	DrawX = std::clamp(DrawX, 0.0f, maximum(0.0f, m_Width - BoxWidth));
	DrawY = std::clamp(DrawY, 0.0f, maximum(0.0f, m_Height - BoxHeight));
	DrawX = std::clamp(DrawX + GameClient()->m_MusicPlayer.GetHudPushOffsetForRect({DrawX, DrawY, BoxWidth, BoxHeight}, m_Width, 4.0f * Scale), 0.0f, maximum(0.0f, m_Width - BoxWidth));
	DrawY = std::clamp(DrawY + GameClient()->m_MusicPlayer.GetHudPushDownOffsetForRect({DrawX, DrawY, BoxWidth, BoxHeight}, m_Height, 4.0f * Scale), 0.0f, maximum(0.0f, m_Height - BoxHeight));

	if(BackgroundEnabled)
	{
		const int CornerMask = DrawY > 0.01f ? IGraphics::CORNER_ALL : IGraphics::CORNER_B;
		const int Corners = HudModuleBackgroundCorners(CornerMask, DrawX, DrawY, BoxWidth, BoxHeight, m_Width, m_Height);
		Graphics()->DrawRect(DrawX, DrawY, BoxWidth, BoxHeight, ApplyHudAlpha(HudModuleBackgroundColor(BackgroundColor)), Corners, 3.75f * Scale);
	}
	TextRender()->Text(DrawX + 5.0f * Scale, DrawY + (BoxHeight - FontSize) / 2.0f, FontSize, aTimeStr, -1.0f);

	// Graphics()->DrawRect(x - 30.0f, 0.0f, 25.0f, 12.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.4f), IGraphics::CORNER_B, 3.75f);
	// TextRender()->Text(x - 25.0f, (12.5f - 5.f) / 2.f, 5.0f, aTimeStr, -1.0f);
}

void CHud::OnNewSnapshot()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	if(!GameClient()->m_Snap.m_pGameInfoObj)
		return;

	int ClientId = -1;
	if(GameClient()->m_Snap.m_pLocalCharacter && !GameClient()->m_Snap.m_SpecInfo.m_Active && !(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER))
		ClientId = GameClient()->m_Snap.m_LocalClientId;
	else if(GameClient()->m_Snap.m_SpecInfo.m_Active)
		ClientId = GameClient()->m_Snap.m_SpecInfo.m_SpectatorId;

	if(ClientId == -1)
		return;

	const CNetObj_Character *pPrevChar = &GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev;
	const CNetObj_Character *pCurChar = &GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur;
	const float IntraTick = Client()->IntraGameTick(g_Config.m_ClDummy);
	ivec2 Vel = mix(ivec2(pPrevChar->m_VelX, pPrevChar->m_VelY), ivec2(pCurChar->m_VelX, pCurChar->m_VelY), IntraTick);

	CCharacter *pChar = GameClient()->m_PredictedWorld.GetCharacterById(ClientId);
	if(pChar && pChar->IsGrounded())
		Vel.y = 0;

	int aVels[2] = {Vel.x, Vel.y};

	for(int i = 0; i < 2; i++)
	{
		int AbsVel = abs(aVels[i]);
		if(AbsVel > m_aPlayerSpeed[i])
		{
			m_aLastPlayerSpeedChange[i] = ESpeedChange::INCREASE;
		}
		if(AbsVel < m_aPlayerSpeed[i])
		{
			m_aLastPlayerSpeedChange[i] = ESpeedChange::DECREASE;
		}
		if(AbsVel < 2)
		{
			m_aLastPlayerSpeedChange[i] = ESpeedChange::NONE;
		}
		m_aPlayerSpeed[i] = AbsVel;
	}
}

void CHud::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	if(!GameClient()->m_Snap.m_pGameInfoObj)
		return;

	m_Width = 300.0f * Graphics()->ScreenAspect();
	m_Height = 300.0f;
	Graphics()->MapScreen(0.0f, 0.0f, m_Width, m_Height);
	SetHudTextColor(TextRender()->DefaultTextColor());
	TextRender()->TextOutlineColor(ApplyHudAlpha(TextRender()->DefaultTextOutlineColor()));

#if defined(CONF_VIDEORECORDER)
	if((IVideo::Current() && g_Config.m_ClVideoShowhud) || (!IVideo::Current() && g_Config.m_ClShowhud))
#else
	if(g_Config.m_ClShowhud)
#endif
	{
		// Check focus mode settings for UI elements
		bool FocusModeActive = g_Config.m_ClFocusMode;
		bool HideHudInFocusMode = g_Config.m_ClFocusModeHideHud;
		bool HideUIInFocusMode = g_Config.m_ClFocusModeHideUI;

		if(FocusModeActive && HideHudInFocusMode)
		{
			GameClient()->m_BestClient.RenderHookCombo();
			RenderCursor();
			return;
		}

		if(GameClient()->m_Snap.m_pLocalCharacter && !GameClient()->m_Snap.m_SpecInfo.m_Active && !(GameClient()->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER))
		{
			if(g_Config.m_ClShowhudHealthAmmo)
			{
				RenderAmmoHealthAndArmor(GameClient()->m_Snap.m_pLocalCharacter);
			}
			if(GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_HasExtendedData && g_Config.m_ClShowhudDDRace && GameClient()->m_GameInfo.m_HudDDRace)
			{
				RenderPlayerState(GameClient()->m_Snap.m_LocalClientId);
			}
			// Hide spectator count in focus mode
			if(!(FocusModeActive && HideUIInFocusMode))
			{
				RenderSpectatorCount();
			}
			RenderMovementInformation();
			RenderDDRaceEffects();
		}
		else if(GameClient()->m_Snap.m_SpecInfo.m_Active)
		{
			int SpectatorId = GameClient()->m_Snap.m_SpecInfo.m_SpectatorId;
			if(SpectatorId != SPEC_FREEVIEW && g_Config.m_ClShowhudHealthAmmo)
			{
				RenderAmmoHealthAndArmor(&GameClient()->m_Snap.m_aCharacters[SpectatorId].m_Cur);
			}
			if(SpectatorId != SPEC_FREEVIEW &&
				GameClient()->m_Snap.m_aCharacters[SpectatorId].m_HasExtendedData &&
				g_Config.m_ClShowhudDDRace &&
				(!GameClient()->m_MultiViewActivated || GameClient()->m_MultiViewShowHud) &&
				GameClient()->m_GameInfo.m_HudDDRace)
			{
				RenderPlayerState(SpectatorId);
			}
			RenderMovementInformation();
			RenderSpectatorHud();
		}

		RenderPracticeModeBanner();
		if(g_Config.m_BcSpeedrunTimer || m_SpeedrunTimerExpiredTick > 0)
			RenderSpeedrunTimer();
		RenderPauseNotification();
		RenderSuddenDeath();
		if(g_Config.m_ClShowhudScore)
			RenderScoreHud();
		// Hide dummy actions in focus mode
		if(!(FocusModeActive && HideUIInFocusMode))
		{
			RenderDummyActions();
			}
			RenderWarmupTimer();
			if(g_Config.m_ClShowhudTimer)
				RenderGameTimer();
			RenderTextInfo();
			RenderLockCam();
			// Hide local time in focus mode
			if(!(FocusModeActive && HideUIInFocusMode))
			{
				RenderLocalTime((m_Width / 7) * 3);
			}
			if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
				RenderConnectionWarning();
			RenderTeambalanceWarning();
			GameClient()->m_Voting.Render();
			if(g_Config.m_ClShowRecord)
				RenderRecord();
			if(!(FocusModeActive && HideUIInFocusMode))
			{
				GameClient()->m_VoiceChat.RenderHudMuteStatusIndicator(m_Width, m_Height);
				GameClient()->m_VoiceChat.RenderHudTalkingIndicator(m_Width, m_Height);
			}
		}
		GameClient()->m_BestClient.RenderHookCombo();
		RenderCursor();
	}

void CHud::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_DDRACETIME || MsgType == NETMSGTYPE_SV_DDRACETIMELEGACY)
	{
		CNetMsg_Sv_DDRaceTime *pMsg = (CNetMsg_Sv_DDRaceTime *)pRawMsg;

		m_DDRaceTime = pMsg->m_Time;

		m_ShowFinishTime = pMsg->m_Finish != 0;

		if(!m_ShowFinishTime)
		{
			m_TimeCpDiff = (float)pMsg->m_Check / 100;
			m_TimeCpLastReceivedTick = Client()->GameTick(g_Config.m_ClDummy);
		}
		else
		{
			m_FinishTimeDiff = (float)pMsg->m_Check / 100;
			m_FinishTimeLastReceivedTick = Client()->GameTick(g_Config.m_ClDummy);
		}
	}
	else if(MsgType == NETMSGTYPE_SV_RECORD || MsgType == NETMSGTYPE_SV_RECORDLEGACY)
	{
		CNetMsg_Sv_Record *pMsg = (CNetMsg_Sv_Record *)pRawMsg;

		// NETMSGTYPE_SV_RACETIME on old race servers
		if(MsgType == NETMSGTYPE_SV_RECORDLEGACY && GameClient()->m_GameInfo.m_DDRaceRecordMessage)
		{
			m_DDRaceTime = pMsg->m_ServerTimeBest; // First value: m_Time

			m_FinishTimeLastReceivedTick = Client()->GameTick(g_Config.m_ClDummy);

			if(pMsg->m_PlayerTimeBest) // Second value: m_Check
			{
				m_TimeCpDiff = (float)pMsg->m_PlayerTimeBest / 100;
				m_TimeCpLastReceivedTick = Client()->GameTick(g_Config.m_ClDummy);
			}
		}
		else if(MsgType == NETMSGTYPE_SV_RECORD || GameClient()->m_GameInfo.m_RaceRecordMessage)
		{
			m_ServerRecord = (float)pMsg->m_ServerTimeBest / 100;
			m_aPlayerRecord[g_Config.m_ClDummy] = (float)pMsg->m_PlayerTimeBest / 100;
		}
	}
}

void CHud::RenderDDRaceEffects()
{
	if(m_DDRaceTime)
	{
		char aBuf[64];
		char aTime[32];
		if(m_ShowFinishTime && m_FinishTimeLastReceivedTick + Client()->GameTickSpeed() * 6 > Client()->GameTick(g_Config.m_ClDummy))
		{
			str_time(m_DDRaceTime, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
			str_format(aBuf, sizeof(aBuf), "Finish time: %s", aTime);

			// calculate alpha (4 sec 1 than get lower the next 2 sec)
			float Alpha = 1.0f;
			if(m_FinishTimeLastReceivedTick + Client()->GameTickSpeed() * 4 < Client()->GameTick(g_Config.m_ClDummy) && m_FinishTimeLastReceivedTick + Client()->GameTickSpeed() * 6 > Client()->GameTick(g_Config.m_ClDummy))
			{
				// lower the alpha slowly to blend text out
				Alpha = ((float)(m_FinishTimeLastReceivedTick + Client()->GameTickSpeed() * 6) - (float)Client()->GameTick(g_Config.m_ClDummy)) / (float)(Client()->GameTickSpeed() * 2);
			}

			SetHudTextColor(1, 1, 1, Alpha);
			CTextCursor Cursor;
			Cursor.SetPosition(vec2(150 * Graphics()->ScreenAspect() - TextRender()->TextWidth(12, aBuf) / 2, 40));
			Cursor.m_FontSize = 12.0f;
			TextRender()->RecreateTextContainer(m_DDRaceEffectsTextContainerIndex, &Cursor, aBuf);
			if(m_FinishTimeDiff != 0.0f && m_DDRaceEffectsTextContainerIndex.Valid())
			{
				if(m_FinishTimeDiff < 0)
				{
					str_time_float(-m_FinishTimeDiff, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
					str_format(aBuf, sizeof(aBuf), "-%s", aTime);
					SetHudTextColor(0.5f, 1.0f, 0.5f, Alpha); // green
				}
				else
				{
					str_time_float(m_FinishTimeDiff, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
					str_format(aBuf, sizeof(aBuf), "+%s", aTime);
					SetHudTextColor(1.0f, 0.5f, 0.5f, Alpha); // red
				}
				CTextCursor DiffCursor;
				DiffCursor.SetPosition(vec2(150 * Graphics()->ScreenAspect() - TextRender()->TextWidth(10, aBuf) / 2, 56));
				DiffCursor.m_FontSize = 10.0f;
				TextRender()->AppendTextContainer(m_DDRaceEffectsTextContainerIndex, &DiffCursor, aBuf);
			}
			if(m_DDRaceEffectsTextContainerIndex.Valid())
			{
				auto OutlineColor = TextRender()->DefaultTextOutlineColor();
				OutlineColor.a *= Alpha;
				TextRender()->RenderTextContainer(m_DDRaceEffectsTextContainerIndex, ApplyHudAlpha(TextRender()->DefaultTextColor()), ApplyHudAlpha(OutlineColor));
			}
			SetHudTextColor(TextRender()->DefaultTextColor());
		}
		else if(g_Config.m_ClShowhudTimeCpDiff && !m_ShowFinishTime && m_TimeCpLastReceivedTick + Client()->GameTickSpeed() * 6 > Client()->GameTick(g_Config.m_ClDummy))
		{
			if(m_TimeCpDiff < 0)
			{
				str_time_float(-m_TimeCpDiff, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
				str_format(aBuf, sizeof(aBuf), "-%s", aTime);
			}
			else
			{
				str_time_float(m_TimeCpDiff, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
				str_format(aBuf, sizeof(aBuf), "+%s", aTime);
			}

			// calculate alpha (4 sec 1 than get lower the next 2 sec)
			float Alpha = 1.0f;
			if(m_TimeCpLastReceivedTick + Client()->GameTickSpeed() * 4 < Client()->GameTick(g_Config.m_ClDummy) && m_TimeCpLastReceivedTick + Client()->GameTickSpeed() * 6 > Client()->GameTick(g_Config.m_ClDummy))
			{
				// lower the alpha slowly to blend text out
				Alpha = ((float)(m_TimeCpLastReceivedTick + Client()->GameTickSpeed() * 6) - (float)Client()->GameTick(g_Config.m_ClDummy)) / (float)(Client()->GameTickSpeed() * 2);
			}

			if(m_TimeCpDiff > 0)
				SetHudTextColor(1.0f, 0.5f, 0.5f, Alpha); // red
			else if(m_TimeCpDiff < 0)
				SetHudTextColor(0.5f, 1.0f, 0.5f, Alpha); // green
			else if(!m_TimeCpDiff)
				SetHudTextColor(1, 1, 1, Alpha); // white

			CTextCursor Cursor;
			Cursor.SetPosition(vec2(150 * Graphics()->ScreenAspect() - TextRender()->TextWidth(10, aBuf) / 2, 34));
			Cursor.m_FontSize = 10.0f;
			TextRender()->RecreateTextContainer(m_DDRaceEffectsTextContainerIndex, &Cursor, aBuf);

			if(m_DDRaceEffectsTextContainerIndex.Valid())
			{
				auto OutlineColor = TextRender()->DefaultTextOutlineColor();
				OutlineColor.a *= Alpha;
				TextRender()->RenderTextContainer(m_DDRaceEffectsTextContainerIndex, ApplyHudAlpha(TextRender()->DefaultTextColor()), ApplyHudAlpha(OutlineColor));
			}
			SetHudTextColor(TextRender()->DefaultTextColor());
		}
	}
}

void CHud::RenderRecord()
{
	if(m_ServerRecord > 0.0f)
	{
		char aBuf[64];
		TextRender()->Text(5, 75, 6, Localize("Server best:"), -1.0f);
		char aTime[32];
		str_time_float(m_ServerRecord, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
		str_format(aBuf, sizeof(aBuf), "%s%s", m_ServerRecord > 3600 ? "" : "   ", aTime);
		TextRender()->Text(53, 75, 6, aBuf, -1.0f);
	}

	const float PlayerRecord = m_aPlayerRecord[g_Config.m_ClDummy];
	if(PlayerRecord > 0.0f)
	{
		char aBuf[64];
		TextRender()->Text(5, 82, 6, Localize("Personal best:"), -1.0f);
		char aTime[32];
		str_time_float(PlayerRecord, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
		str_format(aBuf, sizeof(aBuf), "%s%s", PlayerRecord > 3600 ? "" : "   ", aTime);
		TextRender()->Text(53, 82, 6, aBuf, -1.0f);
	}
}
