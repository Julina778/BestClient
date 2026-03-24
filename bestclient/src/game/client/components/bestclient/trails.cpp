#include "trails.h"

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>

namespace
{
constexpr int TRAIL_MAX_RENDERED_CLIENTS = 12;
constexpr int TRAIL_MAX_SEGMENTS_PER_FRAME = 768;
constexpr int TRAIL_MAX_REMOTE_LENGTH = 96;
}

bool CTrails::ShouldPredictPlayer(int ClientId)
{
	if(!GameClient()->Predict())
		return false;
	CCharacter *pChar = GameClient()->m_PredictedWorld.GetCharacterById(ClientId);
	if(GameClient()->Predict() && (ClientId == GameClient()->m_Snap.m_LocalClientId || (GameClient()->AntiPingPlayers() && !GameClient()->IsOtherTeam(ClientId))) && pChar)
		return true;
	return false;
}

bool CTrails::ShouldRenderClientTrail(int ClientId, bool Local, bool ZoomAllowed) const
{
	if(!Local && !g_Config.m_TcTeeTrailOthers)
		return false;
	if(!Local && !ZoomAllowed)
		return false;
	return GameClient()->m_Snap.m_aCharacters[ClientId].m_Active;
}

void CTrails::ClearAllHistory()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
		ClearHistory(i);
}
void CTrails::ClearHistory(int ClientId)
{
	for(int i = 0; i < 200; ++i)
		m_History[ClientId][i] = {{}, -1};
	m_HistoryValid[ClientId] = false;
}
void CTrails::OnReset()
{
	ClearAllHistory();
}

void CTrails::OnRender()
{
	if(GameClient()->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_VISUALS_TEE_TRAILS))
	{
		ClearAllHistory();
		return;
	}

	// Check focus mode settings
	if(g_Config.m_ClFocusMode && g_Config.m_ClFocusModeHideEffects)
		return;

	if(!g_Config.m_TcTeeTrail)
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	if(!GameClient()->m_Snap.m_pGameInfoObj)
		return;

	Graphics()->TextureClear();
	const bool ZoomAllowed = GameClient()->m_Camera.ZoomAllowed();
	const int GameTick = Client()->GameTick(g_Config.m_ClDummy);
	const int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);

	// Skip expensive trail building/rendering for offscreen players.
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float Border = 400.0f;
	ScreenX0 -= Border;
	ScreenY0 -= Border;
	ScreenX1 += Border;
	ScreenY1 += Border;
	const auto IsVisible = [&](const vec2 &Pos) {
		return Pos.x >= ScreenX0 && Pos.x <= ScreenX1 && Pos.y >= ScreenY0 && Pos.y <= ScreenY1;
	};
	const auto IsSegmentVisible = [&](const vec2 &From, const vec2 &To) {
		if(IsVisible(From) || IsVisible(To))
			return true;
		const float MinX = minimum(From.x, To.x);
		const float MaxX = maximum(From.x, To.x);
		const float MinY = minimum(From.y, To.y);
		const float MaxY = maximum(From.y, To.y);
		return MaxX >= ScreenX0 && MinX <= ScreenX1 && MaxY >= ScreenY0 && MinY <= ScreenY1;
	};
	int RenderedClients = 0;
	int RenderedSegments = 0;

	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		if(RenderedClients >= TRAIL_MAX_RENDERED_CLIENTS || RenderedSegments >= TRAIL_MAX_SEGMENTS_PER_FRAME)
			break;

		const bool Local = GameClient()->m_Snap.m_LocalClientId == ClientId;
		if(!ShouldRenderClientTrail(ClientId, Local, ZoomAllowed))
		{
			if(m_HistoryValid[ClientId])
				ClearHistory(ClientId);
			continue;
		}
		else
			m_HistoryValid[ClientId] = true;

		const CTeeRenderInfo &TeeInfo = GameClient()->m_aClients[ClientId].m_RenderInfo;
		const vec2 RenderPos = GameClient()->m_aClients[ClientId].m_RenderPos;
		if(!Local && !IsVisible(RenderPos))
			continue;
		if(!GameClient()->m_BestClient.OptimizerAllowRenderPos(RenderPos))
		{
			if(m_HistoryValid[ClientId])
				ClearHistory(ClientId);
			continue;
		}

		const bool PredictPlayer = ShouldPredictPlayer(ClientId);
		int StartTick;
		float IntraTick;
		if(PredictPlayer)
		{
			StartTick = PredTick;
			IntraTick = Client()->PredIntraGameTick(g_Config.m_ClDummy);
			if(g_Config.m_TcRemoveAnti)
			{
				StartTick = GameClient()->m_SmoothTick[g_Config.m_ClDummy];
				IntraTick = GameClient()->m_SmoothIntraTick[g_Config.m_ClDummy];
			}
			if(g_Config.m_TcUnpredOthersInFreeze && !Local && Client()->m_IsLocalFrozen)
			{
				StartTick = GameTick;
			}
		}
		else
		{
			StartTick = GameTick;
			IntraTick = Client()->IntraGameTick(g_Config.m_ClDummy);
		}

		const vec2 CurServerPos = vec2(GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur.m_X, GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur.m_Y);
		const vec2 PrevServerPos = vec2(GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev.m_X, GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev.m_Y);
		m_History[ClientId][GameTick % 200] = {
			mix(PrevServerPos, CurServerPos, IntraTick),
			GameTick,
		};

		// // NOTE: this is kind of a hack to fix 25tps. This fixes flickering when using the speed mode
		// m_History[ClientId][(GameTick + 1) % 200] = m_History[ClientId][GameTick % 200];
		// m_History[ClientId][(GameTick + 2) % 200] = m_History[ClientId][GameTick % 200];

		IGraphics::CLineItem LineItem;
		bool LineMode = g_Config.m_TcTeeTrailWidth == 0;

		float Alpha = g_Config.m_TcTeeTrailAlpha / 100.0f;
		// Taken from players.cpp
		if(ClientId == -2)
			Alpha *= g_Config.m_ClRaceGhostAlpha / 100.0f;
		else if(ClientId < 0 || GameClient()->IsOtherTeam(ClientId))
			Alpha *= g_Config.m_ClShowOthersAlpha / 100.0f;

		int TrailLength = g_Config.m_TcTeeTrailLength;
		if(!Local)
			TrailLength = minimum(TrailLength, TRAIL_MAX_REMOTE_LENGTH);
		float Width = g_Config.m_TcTeeTrailWidth;
		int TrailCount = 0;

		// TODO: figure out why this is required
		if(!PredictPlayer)
			TrailLength += 2;
		bool TrailFull = false;
		// Fill trail list with initial positions
		for(int i = 0; i < TrailLength; i++)
		{
			CTrailPart Part;
			int PosTick = StartTick - i;
			if(PredictPlayer)
			{
				if(GameClient()->m_aClients[ClientId].m_aPredTick[PosTick % 200] != PosTick)
					continue;
				Part.m_Pos = GameClient()->m_aClients[ClientId].m_aPredPos[PosTick % 200];
				if(i == TrailLength - 1)
					TrailFull = true;
			}
			else
			{
				if(m_History[ClientId][PosTick % 200].m_Tick != PosTick)
					continue;
				Part.m_Pos = m_History[ClientId][PosTick % 200].m_Pos;
				if(i == TrailLength - 2 || i == TrailLength - 3)
					TrailFull = true;
			}
			Part.m_UnmovedPos = Part.m_Pos;
			Part.m_Tick = PosTick;
			if(TrailCount < (int)m_aTrailScratch.size())
				m_aTrailScratch[TrailCount++] = Part;
		}

		// Trim the ends if intratick is too big
		// this was not trivial to figure out
		int TrimTicks = (int)IntraTick;
		for(int i = 0; i < TrimTicks; i++)
			if(TrailCount > 0)
				--TrailCount;

		// Stuff breaks if we have less than 3 points because we cannot calculate an angle between segments to preserve constant width
		// TODO: Pad the list with generated entries in the same direction as before
		if(TrailCount < 3)
			continue;

		if(PredictPlayer)
			m_aTrailScratch[0].m_Pos = GameClient()->m_aClients[ClientId].m_RenderPos;
		else
			m_aTrailScratch[0].m_Pos = mix(PrevServerPos, CurServerPos, IntraTick);

		if(TrailFull)
			m_aTrailScratch[TrailCount - 1].m_Pos = mix(m_aTrailScratch[TrailCount - 1].m_Pos, m_aTrailScratch[TrailCount - 2].m_Pos, std::fmod(IntraTick, 1.0f));

		// Set progress
		for(int i = 0; i < TrailCount; i++)
		{
			float Size = float(TrailCount - 1 + TrimTicks);
			CTrailPart &Part = m_aTrailScratch[i];
			if(i == 0)
				Part.m_Progress = 0.0f;
			else if(i == TrailCount - 1)
				Part.m_Progress = 1.0f;
			else
				Part.m_Progress = ((float)i + IntraTick - 1.0f) / (Size - 1.0f);

			switch(g_Config.m_TcTeeTrailColorMode)
			{
			case COLORMODE_SOLID:
				Part.m_Col = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_TcTeeTrailColor));
				break;
			case COLORMODE_TEE:
				if(TeeInfo.m_CustomColoredSkin)
					Part.m_Col = TeeInfo.m_ColorBody;
				else
					Part.m_Col = TeeInfo.m_BloodColor;
				break;
			case COLORMODE_RAINBOW:
			{
				float Cycle = (1.0f / TrailLength) * 0.5f;
				float Hue = std::fmod(((Part.m_Tick + 6361 * ClientId) % 1000000) * Cycle, 1.0f);
				Part.m_Col = color_cast<ColorRGBA>(ColorHSLA(Hue, 1.0f, 0.5f));
				break;
			}
			case COLORMODE_SPEED:
			{
				float SpeedSq = 0.0f;
				if(TrailCount > 3)
				{
					float DistSq = 0.0f;
					float TickDiff = 1.0f;
					if(i < 2)
					{
						const vec2 Diff = m_aTrailScratch[i + 2].m_UnmovedPos - Part.m_UnmovedPos;
						DistSq = dot(Diff, Diff);
						TickDiff = (float)std::abs(m_aTrailScratch[i + 2].m_Tick - Part.m_Tick);
					}
					else
					{
						const vec2 Diff = Part.m_UnmovedPos - m_aTrailScratch[i - 2].m_UnmovedPos;
						DistSq = dot(Diff, Diff);
						TickDiff = (float)std::abs(Part.m_Tick - m_aTrailScratch[i - 2].m_Tick);
					}
					if(TickDiff > 0.0f)
						SpeedSq = DistSq / (TickDiff * TickDiff);
				}
				Part.m_Col = color_cast<ColorRGBA>(ColorHSLA(65280 * ((int)(SpeedSq / 12.5f) + 1)).UnclampLighting(ColorHSLA::DARKEST_LGT));
				break;
			}
			default:
				dbg_assert(false, "Invalid value for g_Config.m_TcTeeTrailColorMode");
				dbg_break();
			}

			Part.m_Col.a = Alpha;
			if(g_Config.m_TcTeeTrailFade)
				Part.m_Col.a *= 1.0 - Part.m_Progress;

			Part.m_Width = Width;
			if(g_Config.m_TcTeeTrailTaper)
				Part.m_Width = Width * (1.0 - Part.m_Progress);
		}

		// Remove duplicate elements (those with same Pos)
		int UniqueCount = 0;
		for(int i = 0; i < TrailCount; ++i)
		{
			if(UniqueCount > 0 && m_aTrailScratch[UniqueCount - 1] == m_aTrailScratch[i])
				continue;
			if(UniqueCount != i)
				m_aTrailScratch[UniqueCount] = m_aTrailScratch[i];
			++UniqueCount;
		}
		TrailCount = UniqueCount;

		if(TrailCount < 3)
			continue;

		// Calculate the widths
		for(int i = 0; i < TrailCount; i++)
		{
			CTrailPart &Part = m_aTrailScratch[i];
			vec2 PrevPos;
			vec2 Pos = m_aTrailScratch[i].m_Pos;
			vec2 NextPos;

			if(i == 0)
			{
				vec2 Direction = normalize(m_aTrailScratch[i + 1].m_Pos - Pos);
				PrevPos = Pos - Direction;
			}
			else
				PrevPos = m_aTrailScratch[i - 1].m_Pos;

			if(i == TrailCount - 1)
			{
				vec2 Direction = normalize(Pos - m_aTrailScratch[i - 1].m_Pos);
				NextPos = Pos + Direction;
			}
			else
				NextPos = m_aTrailScratch[i + 1].m_Pos;

			vec2 NextDirection = normalize(NextPos - Pos);
			vec2 PrevDirection = normalize(Pos - PrevPos);

			vec2 Normal = vec2(-PrevDirection.y, PrevDirection.x);
			Part.m_Normal = Normal;
			vec2 Tangent = normalize(NextDirection + PrevDirection);
			if(Tangent == vec2(0.0f, 0.0f))
				Tangent = Normal;

			vec2 PerpVec = vec2(-Tangent.y, Tangent.x);
			Width = Part.m_Width;
			float ScaledWidth = Width / dot(Normal, PerpVec);
			float TopScaled = ScaledWidth;
			float BotScaled = ScaledWidth;
			if(dot(PrevDirection, Tangent) > 0.0f)
				TopScaled = std::min(Width * 3.0f, TopScaled);
			else
				BotScaled = std::min(Width * 3.0f, BotScaled);

			vec2 Top = Pos + PerpVec * TopScaled;
			vec2 Bot = Pos - PerpVec * BotScaled;
			Part.m_Top = Top;
			Part.m_Bot = Bot;

			// Bevel Cap
			if(dot(PrevDirection, NextDirection) < -0.25f)
			{
				Top = Pos + Tangent * Width;
				Bot = Pos - Tangent * Width;

				float Det = PrevDirection.x * NextDirection.y - PrevDirection.y * NextDirection.x;
				if(Det >= 0.0f)
				{
					Part.m_Top = Top;
					Part.m_Bot = Bot;
					if(i > 0)
						m_aTrailScratch[i].m_Flip = true;
				}
				else // <-Left Direction
				{
					Part.m_Top = Bot;
					Part.m_Bot = Top;
					if(i > 0)
						m_aTrailScratch[i].m_Flip = true;
				}
			}
		}

		if(LineMode)
			Graphics()->LinesBegin();
		else
			Graphics()->QuadsBegin();

		bool DrewAnySegment = false;
		// Draw the trail
		for(int i = 0; i < TrailCount - 1; i++)
		{
			if(RenderedSegments >= TRAIL_MAX_SEGMENTS_PER_FRAME)
				break;

			const CTrailPart &Part = m_aTrailScratch[i];
			const CTrailPart &NextPart = m_aTrailScratch[i + 1];
			if(!IsSegmentVisible(Part.m_Pos, NextPart.m_Pos))
				continue;
			const float Dist = distance(Part.m_UnmovedPos, NextPart.m_UnmovedPos);

			const float MaxDiff = 120.0f;
			if(i > 0)
			{
				const CTrailPart &PrevPart = m_aTrailScratch[i - 1];
				float PrevDist = distance(PrevPart.m_UnmovedPos, Part.m_UnmovedPos);
				if(std::abs(Dist - PrevDist) > MaxDiff)
					continue;
			}
			if(i < TrailCount - 2)
			{
				const CTrailPart &NextNextPart = m_aTrailScratch[i + 2];
				float NextDist = distance(NextPart.m_UnmovedPos, NextNextPart.m_UnmovedPos);
				if(std::abs(Dist - NextDist) > MaxDiff)
					continue;
			}

			if(LineMode)
			{
				Graphics()->SetColor(Part.m_Col);
				LineItem = IGraphics::CLineItem(Part.m_Pos.x, Part.m_Pos.y, NextPart.m_Pos.x, NextPart.m_Pos.y);
				Graphics()->LinesDraw(&LineItem, 1);
			}
			else
			{
				vec2 Top, Bot;
				if(Part.m_Flip)
				{
					Top = Part.m_Bot;
					Bot = Part.m_Top;
				}
				else
				{
					Top = Part.m_Top;
					Bot = Part.m_Bot;
				}

				Graphics()->SetColor4(NextPart.m_Col, NextPart.m_Col, Part.m_Col, Part.m_Col);
				// IGraphics::CFreeformItem FreeformItem(Top, Bot, NextPart.m_Top, NextPart.m_Bot);
				IGraphics::CFreeformItem FreeformItem(NextPart.m_Top, NextPart.m_Bot, Top, Bot);

				Graphics()->QuadsDrawFreeform(&FreeformItem, 1);
			}
			DrewAnySegment = true;
			++RenderedSegments;
		}
		if(LineMode)
			Graphics()->LinesEnd();
		else
			Graphics()->QuadsEnd();
		if(DrewAnySegment)
			++RenderedClients;
	}
}
