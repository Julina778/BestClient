/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "menus.h"

#include <base/color.h>
#include <base/log.h>
#include <base/math.h>
#include <base/system.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/client/updater.h>
#include <engine/config.h>
#include <engine/editor.h>
#include <engine/friends.h>
#include <engine/gfx/image_manipulation.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/bc_ui_animations.h>
#include <game/client/components/binds.h>
#include <game/client/components/console.h>
#include <game/client/components/key_binder.h>
#include <game/client/components/menu_background.h>
#include <game/client/components/menus_start.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/ui_listbox.h>
#include <game/localization.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace FontIcons;
using namespace std::chrono_literals;

ColorRGBA CMenus::ms_GuiColor;
ColorRGBA CMenus::ms_ColorTabbarInactiveOutgame;
ColorRGBA CMenus::ms_ColorTabbarActiveOutgame;
ColorRGBA CMenus::ms_ColorTabbarHoverOutgame;
ColorRGBA CMenus::ms_ColorTabbarInactive;
ColorRGBA CMenus::ms_ColorTabbarActive = ColorRGBA(0, 0, 0, 0.5f);
ColorRGBA CMenus::ms_ColorTabbarHover;
ColorRGBA CMenus::ms_ColorTabbarInactiveIngame;
ColorRGBA CMenus::ms_ColorTabbarActiveIngame;
ColorRGBA CMenus::ms_ColorTabbarHoverIngame;

float CMenus::ms_ButtonHeight = 25.0f;
float CMenus::ms_ListheaderHeight = 17.0f;

CMenus::CMenus()
{
	m_Popup = POPUP_NONE;
	m_MenuPage = 0;
	m_GamePage = PAGE_GAME;

	m_NeedRestartGraphics = false;
	m_NeedRestartSound = false;
	m_NeedSendinfo = false;
	m_NeedSendDummyinfo = false;
	m_MenuActive = true;
	m_ShowStart = true;

	str_copy(m_aCurrentDemoFolder, "demos");
	m_DemolistStorageType = IStorage::TYPE_ALL;

	m_DemoPlayerState = DEMOPLAYER_NONE;
	m_Dummy = false;

	for(SUIAnimator &Animator : m_aAnimatorsSettingsTab)
	{
		Animator.m_YOffset = -2.5f;
		Animator.m_HOffset = 5.0f;
		Animator.m_WOffset = 5.0f;
		Animator.m_RepositionLabel = true;
	}

	for(SUIAnimator &Animator : m_aAnimatorsOldSettingsTab)
	{
		Animator.m_YOffset = -2.5f;
		Animator.m_HOffset = 5.0f;
		Animator.m_WOffset = 5.0f;
		Animator.m_RepositionLabel = true;
	}

	for(SUIAnimator &Animator : m_aAnimatorsBigPage)
	{
		Animator.m_YOffset = -5.0f;
		Animator.m_HOffset = 5.0f;
	}

	for(SUIAnimator &Animator : m_aAnimatorsSmallPage)
	{
		Animator.m_YOffset = -2.5f;
		Animator.m_HOffset = 2.5f;
	}

	m_PasswordInput.SetBuffer(g_Config.m_Password, sizeof(g_Config.m_Password));
	m_PasswordInput.SetHidden(true);

	m_FirstLoadComplete = false;

	m_GeneralSubTab = 0;
	m_AppearanceSubTab = 0;
	m_AppearanceTab = APPEARANCE_TAB_HUD;
	m_aMenuSfxLastSample.fill(-1);
	m_aMenuSfxPreferredSample.fill(-1);
	m_aMenuSfxLastPlayTick.fill(0);
}

bool CMenus::FindTClientSettingsPath(char *pBuffer, int BufferSize) const
{
	char aStoragePath[IO_MAX_PATH_LENGTH];
	if(fs_storage_path("DDNet", aStoragePath, sizeof(aStoragePath)) != 0 || aStoragePath[0] == '\0')
		return false;

	str_format(pBuffer, BufferSize, "%s/settings_tclient.cfg", aStoragePath);
	return Storage()->FileExists(pBuffer, IStorage::TYPE_ABSOLUTE);
}

void CMenus::PopupConfirmLoadTClientSettings()
{
	char aPath[IO_MAX_PATH_LENGTH];
	if(!FindTClientSettingsPath(aPath, sizeof(aPath)))
	{
		PopupMessage(TCLocalize("Load tclient settings"), TCLocalize("Could not find settings_tclient.cfg in the DDNet user directory."), Localize("Ok"));
		return;
	}

	if(!Console()->ExecuteFile(aPath, IConsole::CLIENT_ID_UNSPECIFIED, true, IStorage::TYPE_ABSOLUTE))
	{
		PopupMessage(TCLocalize("Load tclient settings"), TCLocalize("Failed to load settings_tclient.cfg."), Localize("Ok"));
	}
}

void CMenus::UnloadMenuSfx()
{
	for(int SampleId : m_vMenuSfxSamples)
	{
		if(SampleId != -1)
			Sound()->UnloadSample(SampleId);
	}

	m_vMenuSfxSamples.clear();
	for(auto &vPool : m_avMenuSfxSamplePools)
		vPool.clear();
	m_aMenuSfxLastSample.fill(-1);
	m_aMenuSfxPreferredSample.fill(-1);
	m_aMenuSfxLastPlayTick.fill(0);
	m_MenuSfxLastPlayTickAny = 0;
	m_MenuSfxLoaded = false;
}

void CMenus::LoadMenuSfx()
{
	UnloadMenuSfx();

	if(!g_Config.m_SndEnable || !Sound()->IsSoundEnabled())
		return;

	struct SMenuSfxScanContext
	{
		std::vector<std::string> m_vFilenames;
		std::unordered_set<std::string> m_SetFilenames;
	};

	std::unordered_map<std::string, int> FilenameToSampleId;
	std::array<std::vector<int>, MENU_SFX_POOL_COUNT> vFallbackPools;
	auto IsUiMenuSoundFilename = [](const std::string &FilenameLower) {
		auto StartsWith = [&FilenameLower](const char *pPrefix) {
			return FilenameLower.rfind(pPrefix, 0) == 0;
		};
		return StartsWith("menu-") ||
		       StartsWith("click-") ||
		       StartsWith("back-") ||
		       StartsWith("check-") ||
		       StartsWith("pause-") ||
		       FilenameLower == "restart.wv" ||
		       FilenameLower == "menuclick.wv" ||
		       FilenameLower == "menuback.wv" ||
		       FilenameLower == "menuhit.wv";
	};

	SMenuSfxScanContext ScanContext;
	Storage()->ListDirectory(IStorage::TYPE_ALL, "audio/osu_lazer_menu", [](const char *pName, int IsDir, int StorageType, void *pUser) {
		(void)StorageType;
		if(IsDir || pName[0] == '.' || !str_endswith(pName, ".wv"))
			return 0;

		auto *pContext = static_cast<SMenuSfxScanContext *>(pUser);
		if(!pContext->m_SetFilenames.emplace(pName).second)
			return 0;
		pContext->m_vFilenames.emplace_back(pName);
		return 0;
	},
		&ScanContext);

	std::sort(ScanContext.m_vFilenames.begin(), ScanContext.m_vFilenames.end());

	for(const std::string &Filename : ScanContext.m_vFilenames)
	{
		std::string FilenameLower(Filename);
		std::transform(FilenameLower.begin(), FilenameLower.end(), FilenameLower.begin(), [](unsigned char c) { return std::tolower(c); });
		if(!IsUiMenuSoundFilename(FilenameLower))
			continue;

		char aPath[IO_MAX_PATH_LENGTH];
		str_format(aPath, sizeof(aPath), "audio/osu_lazer_menu/%s", Filename.c_str());
		const int SampleId = Sound()->LoadWV(aPath, IStorage::TYPE_ALL);
		if(SampleId == -1)
			continue;

		m_vMenuSfxSamples.push_back(SampleId);
		FilenameToSampleId[FilenameLower] = SampleId;

		int Pool = MENU_SFX_POOL_CLICK;
		if(FilenameLower.find("hover") != std::string::npos)
			Pool = MENU_SFX_POOL_HOVER;
		else if(FilenameLower.find("back") != std::string::npos || FilenameLower.find("close") != std::string::npos)
			Pool = MENU_SFX_POOL_BACK;
		vFallbackPools[Pool].push_back(SampleId);
	}

	auto AddCurated = [this, &FilenameToSampleId](int Pool, const char *pFilename) {
		auto It = FilenameToSampleId.find(pFilename);
		if(It == FilenameToSampleId.end())
			return;
		const int SampleId = It->second;
		if(std::find(m_avMenuSfxSamplePools[Pool].begin(), m_avMenuSfxSamplePools[Pool].end(), SampleId) == m_avMenuSfxSamplePools[Pool].end())
			m_avMenuSfxSamplePools[Pool].push_back(SampleId);
	};

	// Curated UI-only set (no taiko/drum/normal/soft/spinner samples).
	AddCurated(MENU_SFX_POOL_HOVER, "check-on.wv");
	AddCurated(MENU_SFX_POOL_HOVER, "menu-back-hover.wv");
	AddCurated(MENU_SFX_POOL_HOVER, "back-button-hover.wv");

	AddCurated(MENU_SFX_POOL_CLICK, "click-short-confirm.wv");
	AddCurated(MENU_SFX_POOL_CLICK, "pause-continue-click.wv");
	AddCurated(MENU_SFX_POOL_CLICK, "click-short.wv");
	AddCurated(MENU_SFX_POOL_CLICK, "menuhit.wv");

	AddCurated(MENU_SFX_POOL_BACK, "check-off.wv");
	AddCurated(MENU_SFX_POOL_BACK, "click-close.wv");
	AddCurated(MENU_SFX_POOL_BACK, "back-button-click.wv");
	AddCurated(MENU_SFX_POOL_BACK, "menu-back-click.wv");

	for(int Pool = 0; Pool < MENU_SFX_POOL_COUNT; Pool++)
	{
		if(m_avMenuSfxSamplePools[Pool].empty())
			m_avMenuSfxSamplePools[Pool] = vFallbackPools[Pool];
	}

	for(int i = 0; i < MENU_SFX_POOL_COUNT; i++)
	{
		if(!m_avMenuSfxSamplePools[i].empty())
			m_aMenuSfxPreferredSample[i] = m_avMenuSfxSamplePools[i][0];
	}

	m_MenuSfxLoaded = !m_vMenuSfxSamples.empty();
}

void CMenus::OnButtonSoundEvent(CUi::EButtonSoundEvent Event)
{
	if(!m_MenuActive || !g_Config.m_BcMenuSfx || !g_Config.m_SndEnable || !Sound()->IsSoundEnabled())
		return;

	if(!m_MenuSfxLoaded)
		LoadMenuSfx();

	if(!m_MenuSfxLoaded)
		return;

	int Pool = MENU_SFX_POOL_CLICK;
	if(Event == CUi::EButtonSoundEvent::HOVER_ENTER)
		Pool = MENU_SFX_POOL_HOVER;
	else if(Event == CUi::EButtonSoundEvent::RIGHT_CLICK || Event == CUi::EButtonSoundEvent::MIDDLE_CLICK)
		Pool = MENU_SFX_POOL_BACK;

	const std::vector<int> *pSamples = &m_avMenuSfxSamplePools[Pool];
	if(pSamples->empty())
	{
		if(!m_avMenuSfxSamplePools[MENU_SFX_POOL_CLICK].empty())
			pSamples = &m_avMenuSfxSamplePools[MENU_SFX_POOL_CLICK];
		else
			pSamples = &m_vMenuSfxSamples;
	}
	if(pSamples->empty())
		return;

	const int64_t Now = time_get();
	const int64_t Freq = time_freq();
	const auto MsToTicks = [Freq](int Ms) {
		return (int64_t)Ms * Freq / 1000;
	};

	// Global guard to avoid bursty UI spam when many controls continuously swap hot item.
	if(m_MenuSfxLastPlayTickAny > 0 && Now - m_MenuSfxLastPlayTickAny < MsToTicks(25))
		return;

	// Hover sounds need stronger throttling, dropdowns and color pickers can emit a lot of hover-enter events.
	const int MinPoolIntervalMs = Pool == MENU_SFX_POOL_HOVER ? 100 : 45;
	if(m_aMenuSfxLastPlayTick[Pool] > 0 && Now - m_aMenuSfxLastPlayTick[Pool] < MsToTicks(MinPoolIntervalMs))
		return;

	const float Volume = std::clamp(g_Config.m_BcMenuSfxVolume / 100.0f, 0.0f, 1.0f);
	if(Volume <= 0.0f)
		return;

	int SampleId = m_aMenuSfxPreferredSample[Pool];
	if(SampleId == -1)
		SampleId = (*pSamples)[0];

	m_aMenuSfxLastSample[Pool] = SampleId;
	m_aMenuSfxLastPlayTick[Pool] = Now;
	m_MenuSfxLastPlayTickAny = Now;
	Sound()->Play(CSounds::CHN_GUI, SampleId, 0, Volume);
}

int CMenus::DoButton_Toggle(const void *pId, int Checked, const CUIRect *pRect, bool Active, const unsigned Flags)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	if(!Active)
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	Graphics()->SelectSprite(Checked ? SPRITE_GUIBUTTON_ON : SPRITE_GUIBUTTON_OFF);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	if(Ui()->HotItem() == pId && Active)
	{
		Graphics()->SelectSprite(SPRITE_GUIBUTTON_HOVER);
		QuadItem = IGraphics::CQuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();

	return Active ? Ui()->DoButtonLogic(pId, Checked, pRect, Flags) : 0;
}

int CMenus::DoButton_Menu(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const unsigned Flags, const char *pImageName, int Corners, float Rounding, float FontFactor, ColorRGBA Color)
{
	return DoButton_MenuEx(pButtonContainer, pText, Checked, pRect, Flags, pImageName, Corners, Rounding, FontFactor, Color, false);
}

int CMenus::DoButton_MenuEx(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const unsigned Flags, const char *pImageName, int Corners, float Rounding, float FontFactor, ColorRGBA Color, bool AlwaysColoredImage)
{
	CUIRect Text = *pRect;

	if(Checked)
		Color = ColorRGBA(0.6f, 0.6f, 0.6f, 0.5f);
	else // BestClient, why was this not here? ig they never use "checked" anywhere important
		Color.a *= Ui()->ButtonColorMul(pButtonContainer);

	pRect->Draw(Color, Corners, Rounding);

	if(pImageName)
	{
		CUIRect Image;
		pRect->VSplitRight(pRect->h * 4.0f, &Text, &Image); // always correct ratio for image

		// render image
		const CMenuImage *pImage = FindMenuImage(pImageName);
		if(pImage)
		{
			Graphics()->TextureSet(Ui()->HotItem() == pButtonContainer ? pImage->m_OrgTexture : pImage->m_GreyTexture);
			Graphics()->WrapClamp();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Image.x, Image.y, Image.w, Image.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			Graphics()->WrapNormal();
		}
	}

	Text.HMargin(pRect->h >= 20.0f ? 2.0f : 1.0f, &Text);
	Text.HMargin((Text.h * FontFactor) / 2.0f, &Text);
	Ui()->DoLabel(&Text, pText, Text.h * CUi::ms_FontmodHeight, TEXTALIGN_MC);

	return Ui()->DoButtonLogic(pButtonContainer, Checked, pRect, Flags);
}

int CMenus::DoButton_MenuTab(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, int Corners, SUIAnimator *pAnimator, const ColorRGBA *pDefaultColor, const ColorRGBA *pActiveColor, const ColorRGBA *pHoverColor, float EdgeRounding, const CCommunityIcon *pCommunityIcon)
{
	const bool MouseInside = Ui()->HotItem() == pButtonContainer;
	const bool SearchHighlight = m_MenuPage == PAGE_SETTINGS && pText != nullptr && SettingsSearchMatchesText(pText);
	CUIRect Rect = *pRect;

	if(pAnimator != nullptr)
	{
		auto Time = time_get_nanoseconds();

		if(pAnimator->m_Time + 100ms < Time)
		{
			pAnimator->m_Value = pAnimator->m_Active ? 1 : 0;
			pAnimator->m_Time = Time;
		}

		pAnimator->m_Active = Checked || MouseInside;

		if(pAnimator->m_Active)
			pAnimator->m_Value = std::clamp<float>(pAnimator->m_Value + (Time - pAnimator->m_Time).count() / (double)std::chrono::nanoseconds(100ms).count(), 0, 1);
		else
			pAnimator->m_Value = std::clamp<float>(pAnimator->m_Value - (Time - pAnimator->m_Time).count() / (double)std::chrono::nanoseconds(100ms).count(), 0, 1);

		Rect.w += pAnimator->m_Value * pAnimator->m_WOffset;
		Rect.h += pAnimator->m_Value * pAnimator->m_HOffset;
		Rect.x += pAnimator->m_Value * pAnimator->m_XOffset;
		Rect.y += pAnimator->m_Value * pAnimator->m_YOffset;

		pAnimator->m_Time = Time;
	}

	if(Checked)
	{
		ColorRGBA ColorMenuTab = ms_ColorTabbarActive;
		if(pActiveColor)
			ColorMenuTab = *pActiveColor;

		Rect.Draw(ColorMenuTab, Corners, EdgeRounding);
	}
	else
	{
		if(MouseInside)
		{
			ColorRGBA HoverColorMenuTab = ms_ColorTabbarHover;
			if(pHoverColor)
				HoverColorMenuTab = *pHoverColor;

			Rect.Draw(HoverColorMenuTab, Corners, EdgeRounding);
		}
		else
		{
			ColorRGBA ColorMenuTab = ms_ColorTabbarInactive;
			if(pDefaultColor)
				ColorMenuTab = *pDefaultColor;

			Rect.Draw(ColorMenuTab, Corners, EdgeRounding);
		}
	}

	if(SearchHighlight)
	{
		CUIRect HighlightRect = Rect;
		HighlightRect.Draw(ColorRGBA(1.0f, 0.9f, 0.25f, 0.18f), Corners, EdgeRounding);
	}

	if(pAnimator != nullptr)
	{
		if(pAnimator->m_RepositionLabel)
		{
			Rect.x += Rect.w - pRect->w + Rect.x - pRect->x;
			Rect.y += Rect.h - pRect->h + Rect.y - pRect->y;
		}

		if(!pAnimator->m_ScaleLabel)
		{
			Rect.w = pRect->w;
			Rect.h = pRect->h;
		}
	}

	if(pCommunityIcon)
	{
		CUIRect CommunityIcon;
		Rect.Margin(2.0f, &CommunityIcon);
		m_CommunityIcons.Render(pCommunityIcon, CommunityIcon, true);
	}
	else
	{
		CUIRect Label;
		Rect.HMargin(2.0f, &Label);
		if(SearchHighlight)
			TextRender()->TextColor(ColorRGBA(1.0f, 0.95f, 0.65f, 1.0f));
		Ui()->DoLabel(&Label, pText, Label.h * CUi::ms_FontmodHeight, TEXTALIGN_MC);
		if(SearchHighlight)
			TextRender()->TextColor(TextRender()->DefaultTextColor());
	}

	return Ui()->DoButtonLogic(pButtonContainer, Checked, pRect, BUTTONFLAG_LEFT);
}

int CMenus::DoButton_GridHeader(const void *pId, const char *pText, int Checked, const CUIRect *pRect, int Align)
{
	if(Checked == 2)
		pRect->Draw(ColorRGBA(1, 0.98f, 0.5f, 0.55f), IGraphics::CORNER_T, 5.0f);
	else if(Checked)
		pRect->Draw(ColorRGBA(1, 1, 1, 0.5f), IGraphics::CORNER_T, 5.0f);

	CUIRect Temp;
	pRect->VMargin(5.0f, &Temp);
	Ui()->DoLabel(&Temp, pText, pRect->h * CUi::ms_FontmodHeight, Align);
	return Ui()->DoButtonLogic(pId, Checked, pRect, BUTTONFLAG_LEFT);
}

int CMenus::DoButton_Favorite(const void *pButtonId, const void *pParentId, bool Checked, const CUIRect *pRect)
{
	if(Checked || (pParentId != nullptr && Ui()->HotItem() == pParentId) || Ui()->HotItem() == pButtonId)
	{
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		const float Alpha = Ui()->HotItem() == pButtonId ? 0.2f : 0.0f;
		TextRender()->TextColor(Checked ? ColorRGBA(1.0f, 0.85f, 0.3f, 0.8f + Alpha) : ColorRGBA(0.5f, 0.5f, 0.5f, 0.8f + Alpha));
		SLabelProperties Props;
		Props.m_MaxWidth = pRect->w;
		Ui()->DoLabel(pRect, FONT_ICON_STAR, 12.0f, TEXTALIGN_MC, Props);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	}
	return Ui()->DoButtonLogic(pButtonId, 0, pRect, BUTTONFLAG_LEFT);
}

int CMenus::DoButton_CheckBox_Common(const void *pId, const char *pText, const char *pBoxText, const CUIRect *pRect, const unsigned Flags)
{
	const bool SearchHighlight = m_MenuPage == PAGE_SETTINGS && pText != nullptr && SettingsSearchMatchesText(pText);
	if(SearchHighlight)
	{
		CUIRect HighlightRect = *pRect;
		HighlightRect.Draw(ColorRGBA(1.0f, 0.9f, 0.25f, 0.16f), IGraphics::CORNER_ALL, 3.0f);
	}

	CUIRect Box, Label;
	pRect->VSplitLeft(pRect->h, &Box, &Label);
	Label.VSplitLeft(5.0f, nullptr, &Label);

	Box.Margin(2.0f, &Box);
	Box.Draw(ColorRGBA(1, 1, 1, 0.25f * Ui()->ButtonColorMul(pId)), IGraphics::CORNER_ALL, 3.0f);

	const bool Checkable = *pBoxText == 'X';
	if(Checkable)
	{
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT);
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		Ui()->DoLabel(&Box, FONT_ICON_XMARK, Box.h * CUi::ms_FontmodHeight, TEXTALIGN_MC);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	}
	else
		Ui()->DoLabel(&Box, pBoxText, Box.h * CUi::ms_FontmodHeight, TEXTALIGN_MC);

	TextRender()->SetRenderFlags(0);
	if(SearchHighlight)
		TextRender()->TextColor(ColorRGBA(1.0f, 0.95f, 0.65f, 1.0f));
	Ui()->DoLabel(&Label, pText, Box.h * CUi::ms_FontmodHeight, TEXTALIGN_ML);
	if(SearchHighlight)
		TextRender()->TextColor(TextRender()->DefaultTextColor());

	return Ui()->DoButtonLogic(pId, 0, pRect, Flags);
}

void CMenus::DoLaserPreview(const CUIRect *pRect, const ColorHSLA LaserOutlineColor, const ColorHSLA LaserInnerColor, const int LaserType)
{
	CUIRect Section = *pRect;
	vec2 From = vec2(Section.x + 30.0f, Section.y + Section.h / 2.0f);
	vec2 Pos = vec2(Section.x + Section.w - 20.0f, Section.y + Section.h / 2.0f);

	const ColorRGBA OuterColor = color_cast<ColorRGBA>(ColorHSLA(LaserOutlineColor));
	const ColorRGBA InnerColor = color_cast<ColorRGBA>(ColorHSLA(LaserInnerColor));
	const float TicksHead = Client()->GlobalTime() * Client()->GameTickSpeed();

	// TicksBody = 4.0 for less laser width for weapon alignment
	GameClient()->m_Items.RenderLaser(From, Pos, OuterColor, InnerColor, 4.0f, TicksHead, LaserType);

	switch(LaserType)
	{
	case LASERTYPE_RIFLE:
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteWeaponLaser);
		Graphics()->SelectSprite(SPRITE_WEAPON_LASER_BODY);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0, 0, 1, 1);
		Graphics()->DrawSprite(Section.x + 30.0f, Section.y + Section.h / 2.0f, 60.0f);
		Graphics()->QuadsEnd();
		break;
	case LASERTYPE_SHOTGUN:
		Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteWeaponShotgun);
		Graphics()->SelectSprite(SPRITE_WEAPON_SHOTGUN_BODY);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0, 0, 1, 1);
		Graphics()->DrawSprite(Section.x + 30.0f, Section.y + Section.h / 2.0f, 60.0f);
		Graphics()->QuadsEnd();
		break;
	case LASERTYPE_DRAGGER:
	{
		CTeeRenderInfo TeeRenderInfo;
		TeeRenderInfo.Apply(GameClient()->m_Skins.Find(g_Config.m_ClPlayerSkin));
		TeeRenderInfo.ApplyColors(g_Config.m_ClPlayerUseCustomColor, g_Config.m_ClPlayerColorBody, g_Config.m_ClPlayerColorFeet);
		TeeRenderInfo.m_Size = 64.0f;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeRenderInfo, EMOTE_NORMAL, vec2(-1, 0), Pos);
		break;
	}
	case LASERTYPE_FREEZE:
	{
		CTeeRenderInfo TeeRenderInfo;
		if(g_Config.m_ClShowNinja)
			TeeRenderInfo.Apply(GameClient()->m_Skins.Find("x_ninja"));
		else
			TeeRenderInfo.Apply(GameClient()->m_Skins.Find(g_Config.m_ClPlayerSkin));
		TeeRenderInfo.m_TeeRenderFlags = TEE_EFFECT_FROZEN;
		TeeRenderInfo.m_Size = 64.0f;
		TeeRenderInfo.m_ColorBody = ColorRGBA(1, 1, 1);
		TeeRenderInfo.m_ColorFeet = ColorRGBA(1, 1, 1);
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeRenderInfo, EMOTE_PAIN, vec2(1, 0), From);
		GameClient()->m_Effects.FreezingFlakes(From, vec2(32, 32), 1.0f);
		break;
	}
	default:
		GameClient()->m_Items.RenderLaser(From, From, OuterColor, InnerColor, 4.0f, TicksHead, LaserType);
	}
}

bool CMenus::DoLine_RadioMenu(CUIRect &View, const char *pLabel, std::vector<CButtonContainer> &vButtonContainers, const std::vector<const char *> &vLabels, const std::vector<int> &vValues, int &Value)
{
	dbg_assert(vButtonContainers.size() == vValues.size(), "vButtonContainers and vValues must have the same size");
	dbg_assert(vButtonContainers.size() == vLabels.size(), "vButtonContainers and vLabels must have the same size");
	const int N = vButtonContainers.size();
	const float Spacing = 2.0f;
	const float ButtonHeight = 20.0f;
	CUIRect Label, Buttons;
	View.HSplitTop(Spacing, nullptr, &View);
	View.HSplitTop(ButtonHeight, &Buttons, &View);
	Buttons.VSplitMid(&Label, &Buttons, 10.0f);
	Buttons.HMargin(2.0f, &Buttons);
	Ui()->DoLabel(&Label, pLabel, 13.0f, TEXTALIGN_ML);
	const float W = Buttons.w / N;
	bool Pressed = false;
	for(int i = 0; i < N; ++i)
	{
		CUIRect Button;
		Buttons.VSplitLeft(W, &Button, &Buttons);
		int Corner = IGraphics::CORNER_NONE;
		if(i == 0)
			Corner = IGraphics::CORNER_L;
		if(i == N - 1)
			Corner = IGraphics::CORNER_R;
		if(DoButton_Menu(&vButtonContainers[i], vLabels[i], vValues[i] == Value, &Button, BUTTONFLAG_LEFT, nullptr, Corner))
		{
			Pressed = true;
			Value = vValues[i];
		}
	}
	return Pressed;
}

ColorHSLA CMenus::DoLine_ColorPicker(CButtonContainer *pResetId, const float LineSize, const float LabelSize, const float BottomMargin, CUIRect *pMainRect, const char *pText, unsigned int *pColorValue, const ColorRGBA DefaultColor, bool CheckBoxSpacing, int *pCheckBoxValue, bool Alpha)
{
	CUIRect Section, ColorPickerButton, ResetButton, Label;

	pMainRect->HSplitTop(LineSize, &Section, pMainRect);
	pMainRect->HSplitTop(BottomMargin, nullptr, pMainRect);

	Section.VSplitRight(60.0f, &Section, &ResetButton);
	Section.VSplitRight(8.0f, &Section, nullptr);
	Section.VSplitRight(Section.h, &Section, &ColorPickerButton);
	Section.VSplitRight(8.0f, &Label, nullptr);

	if(pCheckBoxValue != nullptr)
	{
		Label.Margin(2.0f, &Label);
		if(DoButton_CheckBox(pCheckBoxValue, pText, *pCheckBoxValue, &Label))
			*pCheckBoxValue ^= 1;
	}
	else if(CheckBoxSpacing)
	{
		Label.VSplitLeft(Label.h + 5.0f, nullptr, &Label);
	}
	if(pCheckBoxValue == nullptr)
	{
		Ui()->DoLabel(&Label, pText, LabelSize, TEXTALIGN_ML);
	}

	const ColorHSLA PickedColor = DoButton_ColorPicker(&ColorPickerButton, pColorValue, Alpha);

	ResetButton.HMargin(2.0f, &ResetButton);
	if(DoButton_Menu(pResetId, Localize("Reset"), 0, &ResetButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, 0.1f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f)))
	{
		*pColorValue = color_cast<ColorHSLA>(DefaultColor).Pack(Alpha);
	}

	return PickedColor;
}

ColorHSLA CMenus::DoButton_ColorPicker(const CUIRect *pRect, unsigned int *pHslaColor, bool Alpha)
{
	ColorHSLA HslaColor = ColorHSLA(*pHslaColor, Alpha);

	ColorRGBA Outline = ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f);
	Outline.a *= Ui()->ButtonColorMul(pHslaColor);

	CUIRect Rect;
	pRect->Margin(3.0f, &Rect);

	pRect->Draw(Outline, IGraphics::CORNER_ALL, 4.0f);
	Rect.Draw(color_cast<ColorRGBA>(HslaColor), IGraphics::CORNER_ALL, 4.0f);

	if(Ui()->DoButtonLogic(pHslaColor, 0, pRect, BUTTONFLAG_LEFT))
	{
		m_ColorPickerPopupContext.m_pHslaColor = pHslaColor;
		m_ColorPickerPopupContext.m_HslaColor = HslaColor;
		m_ColorPickerPopupContext.m_HsvaColor = color_cast<ColorHSVA>(HslaColor);
		m_ColorPickerPopupContext.m_RgbaColor = color_cast<ColorRGBA>(m_ColorPickerPopupContext.m_HsvaColor);
		m_ColorPickerPopupContext.m_Alpha = Alpha;
		Ui()->ShowPopupColorPicker(Ui()->MouseX(), Ui()->MouseY(), &m_ColorPickerPopupContext);
	}
	else if(Ui()->IsPopupOpen(&m_ColorPickerPopupContext) && m_ColorPickerPopupContext.m_pHslaColor == pHslaColor)
	{
		HslaColor = color_cast<ColorHSLA>(m_ColorPickerPopupContext.m_HsvaColor);
	}

	return HslaColor;
}

int CMenus::DoButton_CheckBoxAutoVMarginAndSet(const void *pId, const char *pText, int *pValue, CUIRect *pRect, float VMargin)
{
	CUIRect CheckBoxRect;
	pRect->HSplitTop(VMargin, &CheckBoxRect, pRect);

	int Logic = DoButton_CheckBox_Common(pId, pText, *pValue ? "X" : "", &CheckBoxRect, BUTTONFLAG_LEFT);

	if(Logic)
		*pValue ^= 1;

	return Logic;
}

int CMenus::DoButton_CheckBox(const void *pId, const char *pText, int Checked, const CUIRect *pRect)
{
	return DoButton_CheckBox_Common(pId, pText, Checked ? "X" : "", pRect, BUTTONFLAG_LEFT);
}

int CMenus::DoButton_CheckBox_Number(const void *pId, const char *pText, int Checked, const CUIRect *pRect)
{
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", Checked);
	return DoButton_CheckBox_Common(pId, pText, aBuf, pRect, BUTTONFLAG_LEFT | BUTTONFLAG_RIGHT);
}

void CMenus::RenderMenubar(CUIRect Box, IClient::EClientState ClientState)
{
	CUIRect Button;

	int NewPage = -1;
	int ActivePage = -1;
	if(ClientState == IClient::STATE_OFFLINE)
	{
		ActivePage = m_MenuPage;
	}
	else if(ClientState == IClient::STATE_ONLINE)
	{
		ActivePage = m_GamePage;
	}
	else
	{
		dbg_assert_failed("Client state %d is invalid for RenderMenubar", ClientState);
	}

	// First render buttons aligned from right side so remaining
	// width is known when rendering buttons from left side.
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);

	Box.VSplitRight(33.0f, &Box, &Button);
	static CButtonContainer s_QuitButton;
	ColorRGBA QuitColor(1, 0, 0, 0.5f);
	if(DoButton_MenuTab(&s_QuitButton, FONT_ICON_POWER_OFF, 0, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_QUIT], nullptr, nullptr, &QuitColor, 10.0f))
	{
		if(GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
		{
			m_Popup = POPUP_QUIT;
		}
		else
		{
			Client()->Quit();
		}
	}
	GameClient()->m_Tooltips.DoToolTip(&s_QuitButton, &Button, Localize("Quit"));

	Box.VSplitRight(10.0f, &Box, nullptr);
	Box.VSplitRight(33.0f, &Box, &Button);
	static CButtonContainer s_SettingsButton;
	if(DoButton_MenuTab(&s_SettingsButton, FONT_ICON_GEAR, ActivePage == PAGE_SETTINGS, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_SETTINGS]))
	{
		NewPage = PAGE_SETTINGS;
	}
	GameClient()->m_Tooltips.DoToolTip(&s_SettingsButton, &Button, Localize("Settings"));

	Box.VSplitRight(10.0f, &Box, nullptr);
	Box.VSplitRight(33.0f, &Box, &Button);
	static CButtonContainer s_EditorButton;
	if(DoButton_MenuTab(&s_EditorButton, FONT_ICON_PEN_TO_SQUARE, 0, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_EDITOR]))
	{
		g_Config.m_ClEditor = 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&s_EditorButton, &Button, Localize("Editor"));

	if(ClientState == IClient::STATE_OFFLINE)
	{
		Box.VSplitRight(10.0f, &Box, nullptr);
		Box.VSplitRight(33.0f, &Box, &Button);
		static CButtonContainer s_DemoButton;
		if(DoButton_MenuTab(&s_DemoButton, FONT_ICON_CLAPPERBOARD, ActivePage == PAGE_DEMOS, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_DEMOBUTTON]))
		{
			NewPage = PAGE_DEMOS;
		}
		GameClient()->m_Tooltips.DoToolTip(&s_DemoButton, &Button, Localize("Demos"));
		Box.VSplitRight(10.0f, &Box, nullptr);

		Box.VSplitLeft(33.0f, &Button, &Box);

		const bool GotNewsOrUpdate = (bool)g_Config.m_UiUnreadNews;

		ColorRGBA HomeButtonColorAlert(0, 1, 0, 0.25f);
		ColorRGBA HomeButtonColorAlertHover(0, 1, 0, 0.5f);
		ColorRGBA *pHomeButtonColor = nullptr;
		ColorRGBA *pHomeButtonColorHover = nullptr;

		const char *pHomeScreenButtonLabel = FONT_ICON_HOUSE;
		if(GotNewsOrUpdate)
		{
			pHomeScreenButtonLabel = FONT_ICON_NEWSPAPER;
			pHomeButtonColor = &HomeButtonColorAlert;
			pHomeButtonColorHover = &HomeButtonColorAlertHover;
		}

		static CButtonContainer s_StartButton;
		if(DoButton_MenuTab(&s_StartButton, pHomeScreenButtonLabel, false, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_HOME], pHomeButtonColor, pHomeButtonColor, pHomeButtonColorHover, 10.0f))
		{
			m_ShowStart = true;
		}
		GameClient()->m_Tooltips.DoToolTip(&s_StartButton, &Button, Localize("Main menu"));

		const float BrowserButtonWidth = 75.0f;
		Box.VSplitLeft(10.0f, nullptr, &Box);
		Box.VSplitLeft(BrowserButtonWidth, &Button, &Box);
		static CButtonContainer s_InternetButton;
		if(DoButton_MenuTab(&s_InternetButton, FONT_ICON_EARTH_AMERICAS, ActivePage == PAGE_INTERNET, &Button, IGraphics::CORNER_T, &m_aAnimatorsBigPage[BIG_TAB_INTERNET]))
		{
			NewPage = PAGE_INTERNET;
		}
		GameClient()->m_Tooltips.DoToolTip(&s_InternetButton, &Button, Localize("Internet"));

		Box.VSplitLeft(BrowserButtonWidth, &Button, &Box);
		static CButtonContainer s_LanButton;
		if(DoButton_MenuTab(&s_LanButton, FONT_ICON_NETWORK_WIRED, ActivePage == PAGE_LAN, &Button, IGraphics::CORNER_T, &m_aAnimatorsBigPage[BIG_TAB_LAN]))
		{
			NewPage = PAGE_LAN;
		}
		GameClient()->m_Tooltips.DoToolTip(&s_LanButton, &Button, Localize("LAN"));

		Box.VSplitLeft(BrowserButtonWidth, &Button, &Box);
		static CButtonContainer s_FavoritesButton;
		if(DoButton_MenuTab(&s_FavoritesButton, FONT_ICON_STAR, ActivePage == PAGE_FAVORITES, &Button, IGraphics::CORNER_T, &m_aAnimatorsBigPage[BIG_TAB_FAVORITES]))
		{
			NewPage = PAGE_FAVORITES;
		}
		GameClient()->m_Tooltips.DoToolTip(&s_FavoritesButton, &Button, Localize("Favorites"));

		int MaxPage = PAGE_FAVORITES + ServerBrowser()->FavoriteCommunities().size();
		if(
			!Ui()->IsPopupOpen() &&
			CLineInput::GetActiveInput() == nullptr &&
			(g_Config.m_UiPage >= PAGE_INTERNET && g_Config.m_UiPage <= MaxPage) &&
			(m_MenuPage >= PAGE_INTERNET && m_MenuPage <= PAGE_FAVORITE_COMMUNITY_5))
		{
			if(Input()->KeyPress(KEY_RIGHT))
			{
				NewPage = g_Config.m_UiPage + 1;
				if(NewPage > MaxPage)
					NewPage = PAGE_INTERNET;
			}
			if(Input()->KeyPress(KEY_LEFT))
			{
				NewPage = g_Config.m_UiPage - 1;
				if(NewPage < PAGE_INTERNET)
					NewPage = MaxPage;
			}
		}

		size_t FavoriteCommunityIndex = 0;
		static CButtonContainer s_aFavoriteCommunityButtons[5];
		static_assert(std::size(s_aFavoriteCommunityButtons) == (size_t)PAGE_FAVORITE_COMMUNITY_5 - PAGE_FAVORITE_COMMUNITY_1 + 1);
		static_assert(std::size(s_aFavoriteCommunityButtons) == (size_t)BIT_TAB_FAVORITE_COMMUNITY_5 - BIT_TAB_FAVORITE_COMMUNITY_1 + 1);
		static_assert(std::size(s_aFavoriteCommunityButtons) == (size_t)IServerBrowser::TYPE_FAVORITE_COMMUNITY_5 - IServerBrowser::TYPE_FAVORITE_COMMUNITY_1 + 1);
		for(const CCommunity *pCommunity : ServerBrowser()->FavoriteCommunities())
		{
			if(Box.w < BrowserButtonWidth)
				break;
			Box.VSplitLeft(BrowserButtonWidth, &Button, &Box);
			const int Page = PAGE_FAVORITE_COMMUNITY_1 + FavoriteCommunityIndex;
			if(DoButton_MenuTab(&s_aFavoriteCommunityButtons[FavoriteCommunityIndex], FONT_ICON_ELLIPSIS, ActivePage == Page, &Button, IGraphics::CORNER_T, &m_aAnimatorsBigPage[BIT_TAB_FAVORITE_COMMUNITY_1 + FavoriteCommunityIndex], nullptr, nullptr, nullptr, 10.0f, m_CommunityIcons.Find(pCommunity->Id())))
			{
				NewPage = Page;
			}
			GameClient()->m_Tooltips.DoToolTip(&s_aFavoriteCommunityButtons[FavoriteCommunityIndex], &Button, pCommunity->Name());

			++FavoriteCommunityIndex;
			if(FavoriteCommunityIndex >= std::size(s_aFavoriteCommunityButtons))
				break;
		}

		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	}
	else
	{
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

		// online menus
		Box.VSplitLeft(90.0f, &Button, &Box);
		static CButtonContainer s_GameButton;
		if(DoButton_MenuTab(&s_GameButton, Localize("Game"), ActivePage == PAGE_GAME, &Button, IGraphics::CORNER_TL))
			NewPage = PAGE_GAME;

		Box.VSplitLeft(90.0f, &Button, &Box);
		static CButtonContainer s_PlayersButton;
		if(DoButton_MenuTab(&s_PlayersButton, Localize("Players"), ActivePage == PAGE_PLAYERS, &Button, IGraphics::CORNER_NONE))
			NewPage = PAGE_PLAYERS;

		Box.VSplitLeft(130.0f, &Button, &Box);
		static CButtonContainer s_ServerInfoButton;
		if(DoButton_MenuTab(&s_ServerInfoButton, Localize("Server info"), ActivePage == PAGE_SERVER_INFO, &Button, IGraphics::CORNER_NONE))
			NewPage = PAGE_SERVER_INFO;

		Box.VSplitLeft(90.0f, &Button, &Box);
		static CButtonContainer s_NetworkButton;
		if(DoButton_MenuTab(&s_NetworkButton, Localize("Browser"), ActivePage == PAGE_NETWORK, &Button, IGraphics::CORNER_NONE))
			NewPage = PAGE_NETWORK;

		if(GameClient()->m_GameInfo.m_Race)
		{
			Box.VSplitLeft(90.0f, &Button, &Box);
			static CButtonContainer s_GhostButton;
			if(DoButton_MenuTab(&s_GhostButton, Localize("Ghost"), ActivePage == PAGE_GHOST, &Button, IGraphics::CORNER_NONE))
				NewPage = PAGE_GHOST;
		}

		Box.VSplitLeft(100.0f, &Button, &Box);
		Box.VSplitLeft(4.0f, nullptr, &Box);
		static CButtonContainer s_CallVoteButton;
		if(DoButton_MenuTab(&s_CallVoteButton, Localize("Call vote"), ActivePage == PAGE_CALLVOTE, &Button, IGraphics::CORNER_TR))
		{
			NewPage = PAGE_CALLVOTE;
			m_ControlPageOpening = true;
		}

		if(Box.w >= 10.0f + 33.0f + 10.0f)
		{
			TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
			TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);

			Box.VSplitRight(10.0f, &Box, nullptr);
			Box.VSplitRight(33.0f, &Box, &Button);
			static CButtonContainer s_DemoButton;
			if(DoButton_MenuTab(&s_DemoButton, FONT_ICON_CLAPPERBOARD, ActivePage == PAGE_DEMOS, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_DEMOBUTTON]))
			{
				NewPage = PAGE_DEMOS;
			}
			GameClient()->m_Tooltips.DoToolTip(&s_DemoButton, &Button, Localize("Demos"));
			Box.VSplitRight(10.0f, &Box, nullptr);

			TextRender()->SetRenderFlags(0);
			TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		}
	}

	if(NewPage != -1)
	{
		if(ClientState == IClient::STATE_OFFLINE)
			SetMenuPage(NewPage);
		else
			m_GamePage = NewPage;
	}
}

void CMenus::RenderLoading(const char *pCaption, const char *pContent, int IncreaseCounter)
{
	const int CurLoadRenderCount = m_LoadingState.m_Current;
	m_LoadingState.m_Current += IncreaseCounter;
	dbg_assert(m_LoadingState.m_Current <= m_LoadingState.m_Total, "Invalid progress for RenderLoading");

	const std::chrono::nanoseconds Now = time_get_nanoseconds();
	if(Now - m_LoadingState.m_LastRender < std::chrono::nanoseconds(1s) / 60l)
		return;

	m_LoadingState.m_LastRender = Now;
	if(CurLoadRenderCount != m_LoadingState.m_Current || IncreaseCounter == 0)
	{
		// During connect/load phases render the custom loading screen every swap,
		// otherwise one frame can end up as plain black.
		if(Client()->State() == IClient::STATE_CONNECTING || Client()->State() == IClient::STATE_LOADING)
			RenderPopupConnectionStatus(pCaption ? pCaption : "", pContent ? pContent : "");
		Client()->UpdateAndSwap();
	}
}

void CMenus::FinishLoading()
{
	m_LoadingState.m_Current = 0;
	m_LoadingState.m_Total = 0;

	m_FirstLoadComplete = true;
}

void CMenus::RenderNews(CUIRect MainView)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_NEWS);

	g_Config.m_UiUnreadNews = false;

	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	MainView.HSplitTop(10.0f, nullptr, &MainView);
	MainView.VSplitLeft(15.0f, nullptr, &MainView);

	CUIRect Label;

	const char *pStr = Client()->News();
	char aLine[256];
	while((pStr = str_next_token(pStr, "\n", aLine, sizeof(aLine))))
	{
		const int Len = str_length(aLine);
		if(Len > 0 && aLine[0] == '|' && aLine[Len - 1] == '|')
		{
			MainView.HSplitTop(30.0f, &Label, &MainView);
			aLine[Len - 1] = '\0';
			Ui()->DoLabel(&Label, aLine + 1, 20.0f, TEXTALIGN_ML);
		}
		else
		{
			MainView.HSplitTop(20.0f, &Label, &MainView);
			Ui()->DoLabel(&Label, aLine, 15.f, TEXTALIGN_ML);
		}
	}
}

void CMenus::OnInterfacesInit(CGameClient *pClient)
{
	CComponentInterfaces::OnInterfacesInit(pClient);
	m_MenusIngameTouchControls.OnInterfacesInit(pClient);
	m_MenusSettingsControls.OnInterfacesInit(pClient);

	if(!m_pMenusStart)
        m_pMenusStart = new CMenusStart(); 

	m_pMenusStart->OnInterfacesInit(pClient);
	m_CommunityIcons.OnInterfacesInit(pClient);
}

void CMenus::OnInit()
{
	if(g_Config.m_ClShowWelcome)
	{
		m_Popup = POPUP_LANGUAGE;
		m_CreateDefaultFavoriteCommunities = true;
	}

	if(g_Config.m_UiPage >= PAGE_FAVORITE_COMMUNITY_1 && g_Config.m_UiPage <= PAGE_FAVORITE_COMMUNITY_5 &&
		(size_t)(g_Config.m_UiPage - PAGE_FAVORITE_COMMUNITY_1) >= ServerBrowser()->FavoriteCommunities().size())
	{
		// Reset page to internet when there is no favorite community for this page.
		g_Config.m_UiPage = PAGE_INTERNET;
	}

	if(g_Config.m_ClSkipStartMenu)
	{
		m_ShowStart = false;
	}
	m_MenuPage = g_Config.m_UiPage;

	m_RefreshButton.Init(Ui(), -1);
	m_ConnectButton.Init(Ui(), -1);

	Console()->Chain("add_favorite", ConchainFavoritesUpdate, this);
	Console()->Chain("remove_favorite", ConchainFavoritesUpdate, this);
	Console()->Chain("add_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("remove_friend", ConchainFriendlistUpdate, this);

	Console()->Chain("add_excluded_community", ConchainCommunitiesUpdate, this);
	Console()->Chain("remove_excluded_community", ConchainCommunitiesUpdate, this);
	Console()->Chain("add_excluded_country", ConchainCommunitiesUpdate, this);
	Console()->Chain("remove_excluded_country", ConchainCommunitiesUpdate, this);
	Console()->Chain("add_excluded_type", ConchainCommunitiesUpdate, this);
	Console()->Chain("remove_excluded_type", ConchainCommunitiesUpdate, this);

	Console()->Chain("ui_page", ConchainUiPageUpdate, this);

	Console()->Chain("snd_enable", ConchainUpdateMusicState, this);
	Console()->Chain("snd_enable_music", ConchainUpdateMusicState, this);
	Console()->Chain("cl_background_entities", ConchainBackgroundEntities, this);

	Console()->Chain("cl_assets_entities", ConchainAssetsEntities, this);
	Console()->Chain("cl_asset_game", ConchainAssetGame, this);
	Console()->Chain("cl_asset_emoticons", ConchainAssetEmoticons, this);
	Console()->Chain("cl_asset_particles", ConchainAssetParticles, this);
	Console()->Chain("cl_asset_hud", ConchainAssetHud, this);
	Console()->Chain("cl_asset_extras", ConchainAssetExtras, this);
	Console()->Chain("cl_asset_cursor", ConchainAssetCursor, this);
	Console()->Chain("cl_asset_arrow", ConchainAssetArrow, this);
	Console()->Chain("snd_pack", ConchainSndPack, this);

	Console()->Chain("demo_play", ConchainDemoPlay, this);
	Console()->Chain("demo_speed", ConchainDemoSpeed, this);

	m_TextureBlob = Graphics()->LoadTexture("blob.png", IStorage::TYPE_ALL);
	m_MenuMediaBackground.Init(Graphics(), Storage());

	// setup load amount
	m_LoadingState.m_Current = 0;
	m_LoadingState.m_Total = g_pData->m_NumImages + GameClient()->ComponentCount();
	if(!g_Config.m_ClThreadsoundloading)
		m_LoadingState.m_Total += g_pData->m_NumSounds;

	m_IsInit = true;

	// load menu images
	m_vMenuImages.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "menuimages", MenuImageScan, this);

	m_CommunityIcons.Load();

	// Quad for the direction arrows above the player
	m_DirectionQuadContainerIndex = Graphics()->CreateQuadContainer(false);
	Graphics()->QuadContainerAddSprite(m_DirectionQuadContainerIndex, 0.f, 0.f, 22.f);
	Graphics()->QuadContainerUpload(m_DirectionQuadContainerIndex);

	Ui()->SetButtonSoundEventCallback([this](CUi::EButtonSoundEvent Event) {
		OnButtonSoundEvent(Event);
	});
	LoadMenuSfx();

}

void CMenus::ConchainBackgroundEntities(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CMenus *pSelf = (CMenus *)pUserData;
		if(str_comp(g_Config.m_ClBackgroundEntities, pSelf->GameClient()->m_Background.MapName()) != 0)
			pSelf->GameClient()->m_Background.LoadBackground();
	}
}

void CMenus::ConchainUpdateMusicState(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	auto *pSelf = (CMenus *)pUserData;
	if(pResult->NumArguments())
		pSelf->UpdateMusicState();
}

void CMenus::UpdateMusicState()
{
	const bool ShouldPlay = Client()->State() == IClient::STATE_OFFLINE && g_Config.m_SndEnable && g_Config.m_SndMusic;
	if(ShouldPlay && !GameClient()->m_Sounds.IsPlaying(SOUND_MENU))
		GameClient()->m_Sounds.Enqueue(CSounds::CHN_MUSIC, SOUND_MENU);
	else if(!ShouldPlay && GameClient()->m_Sounds.IsPlaying(SOUND_MENU))
		GameClient()->m_Sounds.Stop(SOUND_MENU);
}

void CMenus::PopupMessage(const char *pTitle, const char *pMessage, const char *pButtonLabel, int NextPopup, FPopupButtonCallback pfnButtonCallback)
{
	// reset active item
	Ui()->SetActiveItem(nullptr);

	str_copy(m_aPopupTitle, pTitle);
	str_copy(m_aPopupMessage, pMessage);
	str_copy(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, pButtonLabel);
	m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup = NextPopup;
	m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback = pfnButtonCallback;
	m_Popup = POPUP_MESSAGE;
}

void CMenus::PopupConfirm(const char *pTitle, const char *pMessage, const char *pConfirmButtonLabel, const char *pCancelButtonLabel,
	FPopupButtonCallback pfnConfirmButtonCallback, int ConfirmNextPopup, FPopupButtonCallback pfnCancelButtonCallback, int CancelNextPopup)
{
	// reset active item
	Ui()->SetActiveItem(nullptr);

	str_copy(m_aPopupTitle, pTitle);
	str_copy(m_aPopupMessage, pMessage);
	str_copy(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, pConfirmButtonLabel);
	m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup = ConfirmNextPopup;
	m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback = pfnConfirmButtonCallback;
	str_copy(m_aPopupButtons[BUTTON_CANCEL].m_aLabel, pCancelButtonLabel);
	m_aPopupButtons[BUTTON_CANCEL].m_NextPopup = CancelNextPopup;
	m_aPopupButtons[BUTTON_CANCEL].m_pfnCallback = pfnCancelButtonCallback;
	m_Popup = POPUP_CONFIRM;
}

void CMenus::PopupWarning(const char *pTopic, const char *pBody, const char *pButton, std::chrono::nanoseconds Duration)
{
	// no multiline support for console
	std::string BodyStr = pBody;
	std::replace(BodyStr.begin(), BodyStr.end(), '\n', ' ');
	log_warn("client", "%s: %s", pTopic, BodyStr.c_str());

	Ui()->SetActiveItem(nullptr);

	str_copy(m_aMessageTopic, pTopic);
	str_copy(m_aMessageBody, pBody);
	str_copy(m_aMessageButton, pButton);
	m_Popup = POPUP_WARNING;
	SetActive(true);

	m_PopupWarningDuration = Duration;
	m_PopupWarningLastTime = time_get_nanoseconds();
}

bool CMenus::CanDisplayWarning() const
{
	return m_Popup == POPUP_NONE;
}

void CMenus::Render()
{
	Ui()->MapScreen();
	Ui()->ResetMouseSlow();

	const bool BlockBackgroundButtonSounds = Ui()->IsPopupHovered();
	if(BlockBackgroundButtonSounds)
		Ui()->ClearButtonSoundEventCallback();

	static int s_Frame = 0;
	if(s_Frame == 0)
	{
		RefreshBrowserTab(true);
		s_Frame++;
	}
	else if(s_Frame == 1)
	{
		UpdateMusicState();
		s_Frame++;
	}
	else
	{
		m_CommunityIcons.Update();
	}

	if(ServerBrowser()->DDNetInfoAvailable())
	{
		// Initially add DDNet as favorite community and select its tab.
		// This must be delayed until the DDNet info is available.
		if(m_CreateDefaultFavoriteCommunities)
		{
			m_CreateDefaultFavoriteCommunities = false;
			if(ServerBrowser()->Community(IServerBrowser::COMMUNITY_DDNET) != nullptr)
			{
				ServerBrowser()->FavoriteCommunitiesFilter().Clear();
				ServerBrowser()->FavoriteCommunitiesFilter().Add(IServerBrowser::COMMUNITY_DDNET);
				SetMenuPage(PAGE_FAVORITE_COMMUNITY_1);
				ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITE_COMMUNITY_1);
			}
		}

		if(m_JoinTutorial && m_Popup == POPUP_NONE && !ServerBrowser()->IsGettingServerlist())
		{
			m_JoinTutorial = false;
			// This is only reached on first launch, when the DDNet community tab has been created and
			// activated by default, so the server info for the tutorial server should be available.
			const char *pAddr = ServerBrowser()->GetTutorialServer();
			if(pAddr)
			{
				Client()->Connect(pAddr);
			}
		}
	}

	// Determine the client state once before rendering because it can change
	// while rendering which causes frames with broken user interface.
	const IClient::EClientState ClientState = Client()->State();

	if(ClientState == IClient::STATE_ONLINE || ClientState == IClient::STATE_DEMOPLAYBACK)
	{
		m_MenuMediaBackground.Unload();
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveIngame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveIngame;
		ms_ColorTabbarHover = ms_ColorTabbarHoverIngame;
	}
	else
	{
		const float ScreenHeight = 300.0f;
		const float ScreenWidth = ScreenHeight * Graphics()->ScreenAspect();
		const bool MediaBackgroundDisabled = GameClient()->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_VISUALS_MEDIA_BACKGROUND);
		if(MediaBackgroundDisabled)
			m_MenuMediaBackground.Unload();
		else
		{
			m_MenuMediaBackground.SyncFromConfig();
			m_MenuMediaBackground.Update();
		}
		if((MediaBackgroundDisabled || !m_MenuMediaBackground.Render(ScreenWidth, ScreenHeight)) && !GameClient()->m_MenuBackground.Render())
		{
			RenderBackground();
		}
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveOutgame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveOutgame;
		ms_ColorTabbarHover = ms_ColorTabbarHoverOutgame;
	}

	CUIRect Screen = *Ui()->Screen();
	if(ClientState == IClient::STATE_ONLINE || ClientState == IClient::STATE_DEMOPLAYBACK)
	{
		const bool IngameMenuAnimationEnabled = BCUiAnimations::Enabled() && g_Config.m_BcIngameMenuAnimation != 0 && g_Config.m_BcIngameMenuAnimationMs > 0;
		const float Ease = IngameMenuAnimationEnabled ? BCUiAnimations::EaseInOutQuad(m_BcIngameMenuOpenPhase) : 1.0f;
		const float Slide = (1.0f - Ease) * 60.0f;
		Screen.y -= Slide;
	}
	const CUIRect FullscreenScreen = Screen;
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK || m_Popup != POPUP_NONE)
	{
		Screen.Margin(10.0f, &Screen);
	}

	switch(ClientState)
	{
	case IClient::STATE_QUITTING:
	case IClient::STATE_RESTARTING:
		// Render nothing except menu background. This should not happen for more than one frame.
		return;

	case IClient::STATE_CONNECTING:
		RenderPopupConnecting(Screen);
		break;

	case IClient::STATE_LOADING:
		RenderPopupLoading(Screen);
		break;

	case IClient::STATE_OFFLINE:
		if(m_Popup != POPUP_NONE)
		{
			RenderPopupFullscreen(Screen);
		}
		else if(m_ShowStart && m_pMenusStart)
		{
			m_pMenusStart->RenderStartMenu(Screen);
		}
		else
		{
			CUIRect TabBar, MainView;
			const bool BestClientSettingsVisible = m_MenuPage == PAGE_SETTINGS &&
				((g_Config.m_BcSettingsLayout == 0 && g_Config.m_UiSettingsPage == SETTINGS_BESTCLIENT) ||
					(g_Config.m_BcSettingsLayout == 1 && m_OldSettingsPage == OLD_SETTINGS_BESTCLIENT));
			const bool FullscreenVisualEditor = BestClientSettingsVisible &&
				(m_AssetsEditorState.m_VisualsEditorOpen && m_AssetsEditorState.m_FullscreenOpen);
			const bool FullscreenComponentsEditor = BestClientSettingsVisible &&
				(m_ComponentsEditorState.m_Open && m_ComponentsEditorState.m_FullscreenOpen);
			if(FullscreenVisualEditor || FullscreenComponentsEditor)
				MainView = FullscreenScreen;
			else
				Screen.HSplitTop(24.0f, &TabBar, &MainView);

			if(m_MenuPage == PAGE_NEWS)
			{
				RenderNews(MainView);
			}
			else if(m_MenuPage >= PAGE_INTERNET && m_MenuPage <= PAGE_FAVORITE_COMMUNITY_5)
			{
				RenderServerbrowser(MainView);
			}
			else if(m_MenuPage == PAGE_DEMOS)
			{
				RenderDemoBrowser(MainView);
			}
			else if(m_MenuPage == PAGE_SETTINGS)
			{
				RenderSettings(MainView);
			}
			else
			{
				dbg_assert_failed("Invalid m_MenuPage: %d", m_MenuPage);
			}

			if(!FullscreenVisualEditor && !FullscreenComponentsEditor)
				RenderMenubar(TabBar, ClientState);
		}
		break;

	case IClient::STATE_ONLINE:
		if(m_Popup != POPUP_NONE)
		{
			RenderPopupFullscreen(Screen);
		}
		else
		{
			CUIRect TabBar, MainView;
			const bool BestClientSettingsVisible = m_GamePage == PAGE_SETTINGS &&
				((g_Config.m_BcSettingsLayout == 0 && g_Config.m_UiSettingsPage == SETTINGS_BESTCLIENT) ||
					(g_Config.m_BcSettingsLayout == 1 && m_OldSettingsPage == OLD_SETTINGS_BESTCLIENT));
			const bool FullscreenVisualEditor = BestClientSettingsVisible &&
				(m_AssetsEditorState.m_VisualsEditorOpen && m_AssetsEditorState.m_FullscreenOpen);
			const bool FullscreenComponentsEditor = BestClientSettingsVisible &&
				(m_ComponentsEditorState.m_Open && m_ComponentsEditorState.m_FullscreenOpen);
			if(FullscreenVisualEditor || FullscreenComponentsEditor)
				MainView = FullscreenScreen;
			else
				Screen.HSplitTop(24.0f, &TabBar, &MainView);

			if(m_GamePage == PAGE_GAME)
			{
				RenderGame(MainView);
				RenderIngameHint();
			}
			else if(m_GamePage == PAGE_PLAYERS)
			{
				RenderPlayers(MainView);
			}
			else if(m_GamePage == PAGE_SERVER_INFO)
			{
				RenderServerInfo(MainView);
			}
			else if(m_GamePage == PAGE_NETWORK)
			{
				RenderInGameNetwork(MainView);
			}
			else if(m_GamePage == PAGE_GHOST)
			{
				RenderGhost(MainView);
			}
			else if(m_GamePage == PAGE_CALLVOTE)
			{
				RenderServerControl(MainView);
			}
			else if(m_GamePage == PAGE_DEMOS)
			{
				RenderDemoBrowser(MainView);
			}
			else if(m_GamePage == PAGE_SETTINGS)
			{
				RenderSettings(MainView);
			}
			else
			{
				dbg_assert_failed("Invalid m_GamePage: %d", m_GamePage);
			}

			if(!FullscreenVisualEditor)
				RenderMenubar(TabBar, ClientState);
		}
		break;

	case IClient::STATE_DEMOPLAYBACK:
		if(m_Popup != POPUP_NONE)
		{
			RenderPopupFullscreen(Screen);
		}
		else
		{
			RenderDemoPlayer(Screen);
		}
		break;
	}

	if(BlockBackgroundButtonSounds)
	{
		Ui()->SetButtonSoundEventCallback([this](CUi::EButtonSoundEvent Event) {
			OnButtonSoundEvent(Event);
		});
	}

	Ui()->RenderPopupMenus();

	// Prevent UI elements from being hovered while a key reader is active
	if(GameClient()->m_KeyBinder.IsActive())
	{
		Ui()->SetHotItem(nullptr);
	}

	// Handle this escape hotkey after popup menus
	if(!m_ShowStart && ClientState == IClient::STATE_OFFLINE && Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
	{
		Ui()->SetHotItem(nullptr);
		Ui()->SetActiveItem(nullptr);
		m_ShowStart = true;
	}


}

void CMenus::RenderPopupFullscreen(CUIRect Screen)
{
	const char *pError = Client()->ErrorString();
	if(m_Popup == POPUP_DISCONNECTED && pError && str_comp(pError, "This server is full") == 0)
	{
		RenderPopupServerFull(Screen);
		return;
	}

	char aBuf[1536];
	const char *pTitle = "";
	const char *pExtraText = "";
	const char *pButtonText = "";
	bool TopAlign = false;

	ColorRGBA BgColor = ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f);
	if(m_Popup == POPUP_MESSAGE || m_Popup == POPUP_CONFIRM)
	{
		pTitle = m_aPopupTitle;
		pExtraText = m_aPopupMessage;
		TopAlign = true;
	}
	else if(m_Popup == POPUP_DISCONNECTED)
	{
		pTitle = Localize("Disconnected");
		pExtraText = Client()->ErrorString();
		pButtonText = Localize("Ok");
		if(Client()->ReconnectTime() > 0)
		{
			str_format(aBuf, sizeof(aBuf), Localize("Reconnect in %d sec"), (int)((Client()->ReconnectTime() - time_get()) / time_freq()) + 1);
			pTitle = Client()->ErrorString();
			pExtraText = aBuf;
			pButtonText = Localize("Abort");
		}
	}
	else if(m_Popup == POPUP_RENAME_DEMO)
	{
		dbg_assert(m_DemolistSelectedIndex >= 0, "m_DemolistSelectedIndex invalid for POPUP_RENAME_DEMO");
		pTitle = m_vpFilteredDemos[m_DemolistSelectedIndex]->m_IsDir ? Localize("Rename folder") : Localize("Rename demo");
	}
#if defined(CONF_VIDEORECORDER)
	else if(m_Popup == POPUP_RENDER_DEMO)
	{
		pTitle = Localize("Render demo");
	}
	else if(m_Popup == POPUP_RENDER_DONE)
	{
		pTitle = Localize("Render complete");
	}
#endif
	else if(m_Popup == POPUP_PASSWORD)
	{
		pTitle = Localize("Password incorrect");
		pButtonText = Localize("Try again");
	}
	else if(m_Popup == POPUP_RESTART)
	{
		pTitle = Localize("Restart");
		pExtraText = Localize("Are you sure that you want to restart?");
	}
	else if(m_Popup == POPUP_QUIT)
	{
		pTitle = Localize("Quit");
		pExtraText = Localize("Are you sure that you want to quit?");
	}
	else if(m_Popup == POPUP_FIRST_LAUNCH)
	{
		pTitle = Localize("Welcome to DDNet");
		str_format(aBuf, sizeof(aBuf), "%s\n\n%s\n\n%s\n\n%s",
			Localize("DDraceNetwork is a cooperative online game where the goal is for you and your group of tees to reach the finish line of the map. As a newcomer you should start on Novice servers, which host the easiest maps. Consider the ping to choose a server close to you."),
			Localize("Use k key to kill (restart), q to pause and watch other players. See settings for other key binds."),
			Localize("It's recommended that you check the settings to adjust them to your liking before joining a server."),
			Localize("Please enter your nickname below."));
		pExtraText = aBuf;
		pButtonText = Localize("Ok");
		TopAlign = true;
	}
	else if(m_Popup == POPUP_POINTS)
	{
		pTitle = Localize("Existing Player");
		if(Client()->InfoState() == IClient::EInfoState::SUCCESS && Client()->Points() > 50)
		{
			str_format(aBuf, sizeof(aBuf), Localize("Your nickname '%s' is already used (%d points). Do you still want to use it?"), Client()->PlayerName(), Client()->Points());
			pExtraText = aBuf;
			TopAlign = true;
		}
		else
		{
			pExtraText = Localize("Checking for existing player with your name");
		}
	}
	else if(m_Popup == POPUP_WARNING)
	{
		BgColor = ColorRGBA(0.5f, 0.0f, 0.0f, 0.7f);
		pTitle = m_aMessageTopic;
		pExtraText = m_aMessageBody;
		pButtonText = m_aMessageButton;
		TopAlign = true;
	}
	else if(m_Popup == POPUP_SAVE_SKIN)
	{
		pTitle = Localize("Save skin");
		pExtraText = Localize("Are you sure you want to save your skin? If a skin with this name already exists, it will be replaced.");
	}

	CUIRect Box, Part;
	Box = Screen;
	if(m_Popup != POPUP_FIRST_LAUNCH)
		Box.Margin(150.0f, &Box);

	// render the box
	Box.Draw(BgColor, IGraphics::CORNER_ALL, 15.0f);

	Box.HSplitTop(20.f, &Part, &Box);
	Box.HSplitTop(24.f, &Part, &Box);
	Part.VMargin(20.f, &Part);
	SLabelProperties Props;
	Props.m_MaxWidth = (int)Part.w;

	if(TextRender()->TextWidth(24.f, pTitle, -1, -1.0f) > Part.w)
		Ui()->DoLabel(&Part, pTitle, 24.f, TEXTALIGN_ML, Props);
	else
		Ui()->DoLabel(&Part, pTitle, 24.f, TEXTALIGN_MC);

	Box.HSplitTop(20.f, &Part, &Box);
	Box.HSplitTop(24.f, &Part, &Box);
	Part.VMargin(20.f, &Part);

	float FontSize = m_Popup == POPUP_FIRST_LAUNCH ? 16.0f : 20.f;

	Props.m_MaxWidth = (int)Part.w;
	if(TopAlign)
		Ui()->DoLabel(&Part, pExtraText, FontSize, TEXTALIGN_TL, Props);
	else if(TextRender()->TextWidth(FontSize, pExtraText, -1, -1.0f) > Part.w)
		Ui()->DoLabel(&Part, pExtraText, FontSize, TEXTALIGN_ML, Props);
	else
		Ui()->DoLabel(&Part, pExtraText, FontSize, TEXTALIGN_MC);

	if(m_Popup == POPUP_MESSAGE || m_Popup == POPUP_CONFIRM)
	{
		CUIRect ButtonBar;
		Box.HSplitBottom(20.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &ButtonBar);
		ButtonBar.VMargin(100.0f, &ButtonBar);

		if(m_Popup == POPUP_MESSAGE)
		{
			static CButtonContainer s_ButtonConfirm;
			if(DoButton_Menu(&s_ButtonConfirm, m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, 0, &ButtonBar) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
			{
				m_Popup = m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup;
				(this->*m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback)();
			}
		}
		else if(m_Popup == POPUP_CONFIRM)
		{
			CUIRect CancelButton, ConfirmButton;
			ButtonBar.VSplitMid(&CancelButton, &ConfirmButton, 40.0f);

			static CButtonContainer s_ButtonCancel;
			if(DoButton_Menu(&s_ButtonCancel, m_aPopupButtons[BUTTON_CANCEL].m_aLabel, 0, &CancelButton) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
			{
				m_Popup = m_aPopupButtons[BUTTON_CANCEL].m_NextPopup;
				(this->*m_aPopupButtons[BUTTON_CANCEL].m_pfnCallback)();
			}

			static CButtonContainer s_ButtonConfirm;
			if(DoButton_Menu(&s_ButtonConfirm, m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, 0, &ConfirmButton) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
			{
				m_Popup = m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup;
				(this->*m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback)();
			}
		}
	}
	else if(m_Popup == POPUP_QUIT || m_Popup == POPUP_RESTART)
	{
		CUIRect Yes, No;
		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);

		// additional info
		Box.VMargin(20.f, &Box);
		if(GameClient()->Editor()->HasUnsavedData())
		{
			str_format(aBuf, sizeof(aBuf), "%s\n\n%s", Localize("There's an unsaved map in the editor, you might want to save it."), Localize("Continue anyway?"));
			Props.m_MaxWidth = Part.w - 20.0f;
			Ui()->DoLabel(&Box, aBuf, 20.f, TEXTALIGN_ML, Props);
		}

		// buttons
		Part.VMargin(80.0f, &Part);
		Part.VSplitMid(&No, &Yes);
		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonTryAgain;
		if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			if(m_Popup == POPUP_RESTART)
			{
				m_Popup = POPUP_NONE;
				Client()->Restart();
			}
			else
			{
				m_Popup = POPUP_NONE;
				Client()->Quit();
			}
		}
	}
	else if(m_Popup == POPUP_PASSWORD)
	{
		Box.HSplitBottom(20.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &Part);
		Part.VMargin(100.0f, &Part);

		CUIRect TryAgain, Abort;
		Part.VSplitMid(&Abort, &TryAgain, 40.0f);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) ||
			Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		{
			m_Popup = POPUP_NONE;
		}

		char aAddr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Client()->ServerAddress(), aAddr, sizeof(aAddr), true);

		static CButtonContainer s_ButtonTryAgain;
		if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) ||
			Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			Client()->Connect(aAddr, g_Config.m_Password);
		}

		Box.VMargin(60.0f, &Box);
		Box.HSplitBottom(32.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &Part);

		CUIRect Label, TextBox;
		Part.VSplitLeft(100.0f, &Label, &TextBox);
		TextBox.VSplitLeft(20.0f, nullptr, &TextBox);
		Ui()->DoLabel(&Label, Localize("Password"), 18.0f, TEXTALIGN_ML);
		Ui()->DoClearableEditBox(&m_PasswordInput, &TextBox, 12.0f);

		Box.HSplitBottom(32.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &Part);

		CUIRect Address;
		Part.VSplitLeft(100.0f, &Label, &Address);
		Address.VSplitLeft(20.0f, nullptr, &Address);
		Ui()->DoLabel(&Label, Localize("Address"), 18.0f, TEXTALIGN_ML);
		Ui()->DoLabel(&Address, aAddr, 18.0f, TEXTALIGN_ML);

		const CServerBrowser::CServerEntry *pEntry = ServerBrowser()->Find(Client()->ServerAddress());
		if(pEntry != nullptr && pEntry->m_GotInfo)
		{
			const CCommunity *pCommunity = ServerBrowser()->Community(pEntry->m_Info.m_aCommunityId);
			const CCommunityIcon *pIcon = pCommunity == nullptr ? nullptr : m_CommunityIcons.Find(pCommunity->Id());

			Box.HSplitBottom(32.0f, &Box, nullptr);
			Box.HSplitBottom(24.0f, &Box, &Part);

			CUIRect Name;
			Part.VSplitLeft(100.0f, &Label, &Name);
			Name.VSplitLeft(20.0f, nullptr, &Name);
			if(pIcon != nullptr)
			{
				CUIRect Icon;
				static char s_CommunityTooltipButtonId;
				Name.VSplitLeft(2.5f * Name.h, &Icon, &Name);
				m_CommunityIcons.Render(pIcon, Icon, true);
				Ui()->DoButtonLogic(&s_CommunityTooltipButtonId, 0, &Icon, BUTTONFLAG_NONE);
				GameClient()->m_Tooltips.DoToolTip(&s_CommunityTooltipButtonId, &Icon, pCommunity->Name());
			}

			Ui()->DoLabel(&Label, Localize("Name"), 18.0f, TEXTALIGN_ML);
			Ui()->DoLabel(&Name, pEntry->m_Info.m_aName, 18.0f, TEXTALIGN_ML);
		}
	}
	else if(m_Popup == POPUP_LANGUAGE)
	{
		CUIRect Button;
		Screen.Margin(150.0f, &Box);
		Box.HSplitTop(20.0f, nullptr, &Box);
		Box.HSplitBottom(20.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &Button);
		Box.HSplitBottom(20.0f, &Box, nullptr);
		Box.VMargin(20.0f, &Box);
		const bool Activated = RenderLanguageSelection(Box);
		Button.VMargin(120.0f, &Button);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, Localize("Ok"), 0, &Button) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || Activated)
			m_Popup = POPUP_FIRST_LAUNCH;
	}
	else if(m_Popup == POPUP_RENAME_DEMO)
	{
		CUIRect Label, TextBox, Ok, Abort;

		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&Abort, &Ok);

		Ok.VMargin(20.0f, &Ok);
		Abort.VMargin(20.0f, &Abort);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonOk;
		if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Ok) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			m_Popup = POPUP_NONE;
			// rename demo
			char aBufOld[IO_MAX_PATH_LENGTH];
			str_format(aBufOld, sizeof(aBufOld), "%s/%s", m_aCurrentDemoFolder, m_vpFilteredDemos[m_DemolistSelectedIndex]->m_aFilename);
			char aBufNew[IO_MAX_PATH_LENGTH];
			str_format(aBufNew, sizeof(aBufNew), "%s/%s", m_aCurrentDemoFolder, m_DemoRenameInput.GetString());
			if(!m_vpFilteredDemos[m_DemolistSelectedIndex]->m_IsDir && !str_endswith(aBufNew, ".demo"))
				str_append(aBufNew, ".demo");

			if(str_comp(aBufOld, aBufNew) == 0)
			{
				// Nothing to rename, also same capitalization
			}
			else if(!str_valid_filename(m_DemoRenameInput.GetString()))
			{
				PopupMessage(Localize("Error"), Localize("This name cannot be used for files and folders"), Localize("Ok"), POPUP_RENAME_DEMO);
			}
			else if(str_utf8_comp_nocase(aBufOld, aBufNew) != 0 && // Allow renaming if it only changes capitalization to support case-insensitive filesystems
				Storage()->FileExists(aBufNew, m_vpFilteredDemos[m_DemolistSelectedIndex]->m_StorageType))
			{
				PopupMessage(Localize("Error"), Localize("A demo with this name already exists"), Localize("Ok"), POPUP_RENAME_DEMO);
			}
			else if(Storage()->FolderExists(aBufNew, m_vpFilteredDemos[m_DemolistSelectedIndex]->m_StorageType))
			{
				PopupMessage(Localize("Error"), Localize("A folder with this name already exists"), Localize("Ok"), POPUP_RENAME_DEMO);
			}
			else if(Storage()->RenameFile(aBufOld, aBufNew, m_vpFilteredDemos[m_DemolistSelectedIndex]->m_StorageType))
			{
				str_copy(m_aCurrentDemoSelectionName, m_DemoRenameInput.GetString());
				if(!m_vpFilteredDemos[m_DemolistSelectedIndex]->m_IsDir)
					fs_split_file_extension(m_DemoRenameInput.GetString(), m_aCurrentDemoSelectionName, sizeof(m_aCurrentDemoSelectionName));
				DemolistPopulate();
				DemolistOnUpdate(false);
			}
			else
			{
				PopupMessage(Localize("Error"), m_vpFilteredDemos[m_DemolistSelectedIndex]->m_IsDir ? Localize("Unable to rename the folder") : Localize("Unable to rename the demo"), Localize("Ok"), POPUP_RENAME_DEMO);
			}
		}

		Box.HSplitBottom(60.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);

		Part.VSplitLeft(60.0f, nullptr, &Label);
		Label.VSplitLeft(120.0f, nullptr, &TextBox);
		TextBox.VSplitLeft(20.0f, nullptr, &TextBox);
		TextBox.VSplitRight(60.0f, &TextBox, nullptr);
		Ui()->DoLabel(&Label, Localize("New name:"), 18.0f, TEXTALIGN_ML);
		Ui()->DoEditBox(&m_DemoRenameInput, &TextBox, 12.0f);
	}
#if defined(CONF_VIDEORECORDER)
	else if(m_Popup == POPUP_RENDER_DEMO)
	{
		CUIRect Row, Ok, Abort;
		Box.VMargin(60.0f, &Box);
		Box.HMargin(20.0f, &Box);
		Box.HSplitBottom(24.0f, &Box, &Row);
		Box.HSplitBottom(40.0f, &Box, nullptr);
		Row.VMargin(40.0f, &Row);
		Row.VSplitMid(&Abort, &Ok, 40.0f);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		{
			m_DemoRenderInput.Clear();
			m_Popup = POPUP_NONE;
		}

		static CButtonContainer s_ButtonOk;
		if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Ok) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			m_Popup = POPUP_NONE;
			// render video
			char aVideoPath[IO_MAX_PATH_LENGTH];
			str_format(aVideoPath, sizeof(aVideoPath), "videos/%s", m_DemoRenderInput.GetString());
			if(!str_endswith(aVideoPath, ".mp4"))
				str_append(aVideoPath, ".mp4");

			if(!str_valid_filename(m_DemoRenderInput.GetString()))
			{
				PopupMessage(Localize("Error"), Localize("This name cannot be used for files and folders"), Localize("Ok"), POPUP_RENDER_DEMO);
			}
			else if(Storage()->FolderExists(aVideoPath, IStorage::TYPE_SAVE))
			{
				PopupMessage(Localize("Error"), Localize("A folder with this name already exists"), Localize("Ok"), POPUP_RENDER_DEMO);
			}
			else if(Storage()->FileExists(aVideoPath, IStorage::TYPE_SAVE))
			{
				char aMessage[128 + IO_MAX_PATH_LENGTH];
				str_format(aMessage, sizeof(aMessage), Localize("File '%s' already exists, do you want to overwrite it?"), m_DemoRenderInput.GetString());
				PopupConfirm(Localize("Replace video"), aMessage, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDemoReplaceVideo, POPUP_NONE, &CMenus::DefaultButtonCallback, POPUP_RENDER_DEMO);
			}
			else
			{
				PopupConfirmDemoReplaceVideo();
			}
		}

		CUIRect ShowChatCheckbox, UseSoundsCheckbox;
		Box.HSplitBottom(20.0f, &Box, &Row);
		Box.HSplitBottom(10.0f, &Box, nullptr);
		Row.VSplitMid(&ShowChatCheckbox, &UseSoundsCheckbox, 20.0f);

		if(DoButton_CheckBox(&g_Config.m_ClVideoShowChat, Localize("Show chat"), g_Config.m_ClVideoShowChat, &ShowChatCheckbox))
			g_Config.m_ClVideoShowChat ^= 1;

		if(DoButton_CheckBox(&g_Config.m_ClVideoSndEnable, Localize("Use sounds"), g_Config.m_ClVideoSndEnable, &UseSoundsCheckbox))
			g_Config.m_ClVideoSndEnable ^= 1;

		CUIRect ShowHudButton;
		Box.HSplitBottom(20.0f, &Box, &Row);
		Row.VSplitMid(&Row, &ShowHudButton, 20.0f);

		if(DoButton_CheckBox(&g_Config.m_ClVideoShowhud, Localize("Show ingame HUD"), g_Config.m_ClVideoShowhud, &ShowHudButton))
			g_Config.m_ClVideoShowhud ^= 1;

		// slowdown
		CUIRect SlowDownButton;
		Row.VSplitLeft(20.0f, &SlowDownButton, &Row);
		Row.VSplitLeft(5.0f, nullptr, &Row);
		static CButtonContainer s_SlowDownButton;
		if(Ui()->DoButton_FontIcon(&s_SlowDownButton, FONT_ICON_BACKWARD, 0, &SlowDownButton, BUTTONFLAG_LEFT))
			m_Speed = std::clamp(m_Speed - 1, 0, (int)(std::size(DEMO_SPEEDS) - 1));

		// paused
		CUIRect PausedButton;
		Row.VSplitLeft(20.0f, &PausedButton, &Row);
		Row.VSplitLeft(5.0f, nullptr, &Row);
		static CButtonContainer s_PausedButton;
		if(Ui()->DoButton_FontIcon(&s_PausedButton, FONT_ICON_PAUSE, 0, &PausedButton, BUTTONFLAG_LEFT))
			m_StartPaused ^= 1;

		// fastforward
		CUIRect FastForwardButton;
		Row.VSplitLeft(20.0f, &FastForwardButton, &Row);
		Row.VSplitLeft(8.0f, nullptr, &Row);
		static CButtonContainer s_FastForwardButton;
		if(Ui()->DoButton_FontIcon(&s_FastForwardButton, FONT_ICON_FORWARD, 0, &FastForwardButton, BUTTONFLAG_LEFT))
			m_Speed = std::clamp(m_Speed + 1, 0, (int)(std::size(DEMO_SPEEDS) - 1));

		// speed meter
		char aBuffer[128];
		const char *pPaused = m_StartPaused ? Localize("(paused)") : "";
		str_format(aBuffer, sizeof(aBuffer), "%s: ×%g %s", Localize("Speed"), DEMO_SPEEDS[m_Speed], pPaused);
		Ui()->DoLabel(&Row, aBuffer, 12.8f, TEXTALIGN_ML);
		Box.HSplitBottom(16.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &Row);

		CUIRect Label, TextBox;
		Row.VSplitLeft(110.0f, &Label, &TextBox);
		TextBox.VSplitLeft(10.0f, nullptr, &TextBox);
		Ui()->DoLabel(&Label, Localize("Video name:"), 12.8f, TEXTALIGN_ML);
		Ui()->DoEditBox(&m_DemoRenderInput, &TextBox, 12.8f);

		// Warn about disconnect if online
		if(Client()->State() == IClient::STATE_ONLINE)
		{
			Box.HSplitBottom(10.0f, &Box, nullptr);
			Box.HSplitBottom(20.0f, &Box, &Row);
			SLabelProperties LabelProperties;
			LabelProperties.SetColor(ColorRGBA(1.0f, 0.0f, 0.0f));
			Ui()->DoLabel(&Row, Localize("You will be disconnected from the server."), 12.8f, TEXTALIGN_MC, LabelProperties);
		}
	}
	else if(m_Popup == POPUP_RENDER_DONE)
	{
		CUIRect Ok, OpenFolder;

		char aFilePath[IO_MAX_PATH_LENGTH];
		char aSaveFolder[IO_MAX_PATH_LENGTH];
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, "videos", aSaveFolder, sizeof(aSaveFolder));
		str_format(aFilePath, sizeof(aFilePath), "%s/%s.mp4", aSaveFolder, m_DemoRenderInput.GetString());

		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&OpenFolder, &Ok);

		Ok.VMargin(20.0f, &Ok);
		OpenFolder.VMargin(20.0f, &OpenFolder);

		static CButtonContainer s_ButtonOpenFolder;
		if(DoButton_Menu(&s_ButtonOpenFolder, Localize("Videos directory"), 0, &OpenFolder))
		{
			Client()->ViewFile(aSaveFolder);
		}

		static CButtonContainer s_ButtonOk;
		if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Ok) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			m_Popup = POPUP_NONE;
			m_DemoRenderInput.Clear();
		}

		Box.HSplitBottom(160.f, &Box, &Part);
		Part.VMargin(20.0f, &Part);

		str_format(aBuf, sizeof(aBuf), Localize("Video was saved to '%s'"), aFilePath);

		SLabelProperties MessageProps;
		MessageProps.m_MaxWidth = (int)Part.w;
		Ui()->DoLabel(&Part, aBuf, 18.0f, TEXTALIGN_TL, MessageProps);
	}
#endif
	else if(m_Popup == POPUP_FIRST_LAUNCH)
	{
		CUIRect Label, TextBox, Skip, Join;

		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);
		Part.VMargin(80.0f, &Part);
		Part.VSplitMid(&Skip, &Join);
		Skip.VMargin(20.0f, &Skip);
		Join.VMargin(20.0f, &Join);

		static CButtonContainer s_JoinTutorialButton;
		if(DoButton_Menu(&s_JoinTutorialButton, Localize("Join Tutorial Server"), 0, &Join) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			m_JoinTutorial = true;
			Client()->RequestDDNetInfo();
			m_Popup = g_Config.m_BrIndicateFinished ? POPUP_POINTS : POPUP_NONE;
		}

		static CButtonContainer s_SkipTutorialButton;
		if(DoButton_Menu(&s_SkipTutorialButton, Localize("Skip Tutorial"), 0, &Skip) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		{
			m_JoinTutorial = false;
			Client()->RequestDDNetInfo();
			m_Popup = g_Config.m_BrIndicateFinished ? POPUP_POINTS : POPUP_NONE;
		}

		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);

		Part.VSplitLeft(30.0f, nullptr, &Part);
		str_format(aBuf, sizeof(aBuf), "%s\n(%s)",
			Localize("Show DDNet map finishes in server browser"),
			Localize("transmits your player name to info.ddnet.org"));

		if(DoButton_CheckBox(&g_Config.m_BrIndicateFinished, aBuf, g_Config.m_BrIndicateFinished, &Part))
			g_Config.m_BrIndicateFinished ^= 1;

		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);

		Part.VSplitLeft(60.0f, nullptr, &Label);
		Label.VSplitLeft(100.0f, nullptr, &TextBox);
		TextBox.VSplitLeft(20.0f, nullptr, &TextBox);
		TextBox.VSplitRight(60.0f, &TextBox, nullptr);
		Ui()->DoLabel(&Label, Localize("Nickname"), 16.0f, TEXTALIGN_ML);
		static CLineInput s_PlayerNameInput(g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName));
		s_PlayerNameInput.SetEmptyText(Client()->PlayerName());
		Ui()->DoEditBox(&s_PlayerNameInput, &TextBox, 12.0f);
	}
	else if(m_Popup == POPUP_POINTS)
	{
		Box.HSplitBottom(20.0f, &Box, nullptr);
		Box.HSplitBottom(24.0f, &Box, &Part);
		Part.VMargin(120.0f, &Part);

		if(Client()->InfoState() == IClient::EInfoState::SUCCESS && Client()->Points() > 50)
		{
			CUIRect Yes, No;
			Part.VSplitMid(&No, &Yes, 40.0f);
			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) ||
				Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
			{
				m_Popup = POPUP_FIRST_LAUNCH;
			}

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), 0, &Yes) ||
				Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
			{
				m_Popup = POPUP_NONE;
			}
		}
		else
		{
			static CButtonContainer s_Button;
			if(DoButton_Menu(&s_Button, Localize("Cancel"), 0, &Part) ||
				Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE) ||
				Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) ||
				Client()->InfoState() == IClient::EInfoState::SUCCESS)
			{
				m_Popup = POPUP_NONE;
			}
			if(Client()->InfoState() == IClient::EInfoState::ERROR)
			{
				PopupMessage(Localize("Error checking player name"), Localize("Could not check for existing player with your name. Check your internet connection."), Localize("Ok"));
			}
		}
	}
	else if(m_Popup == POPUP_WARNING)
	{
		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);
		Part.VMargin(120.0f, &Part);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || (m_PopupWarningDuration > 0s && time_get_nanoseconds() - m_PopupWarningLastTime >= m_PopupWarningDuration))
		{
			m_Popup = POPUP_NONE;
			SetActive(false);
		}
	}
	else if(m_Popup == POPUP_SAVE_SKIN)
	{
		CUIRect Label, TextBox, Yes, No;

		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&No, &Yes);

		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		static CButtonContainer s_ButtonNo;
		if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonYes;
		if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), m_SkinNameInput.IsEmpty() ? 1 : 0, &Yes) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			if(!str_valid_filename(m_SkinNameInput.GetString()))
			{
				PopupMessage(Localize("Error"), Localize("This name cannot be used for files and folders"), Localize("Ok"), POPUP_SAVE_SKIN);
			}
			else if(CSkins7::IsSpecialSkin(m_SkinNameInput.GetString()))
			{
				PopupMessage(Localize("Error"), Localize("Unable to save the skin with a reserved name"), Localize("Ok"), POPUP_SAVE_SKIN);
			}
			else if(!GameClient()->m_Skins7.SaveSkinfile(m_SkinNameInput.GetString(), m_Dummy))
			{
				PopupMessage(Localize("Error"), Localize("Unable to save the skin"), Localize("Ok"), POPUP_SAVE_SKIN);
			}
			else
			{
				m_Popup = POPUP_NONE;
				m_SkinList7LastRefreshTime = std::nullopt;
			}
		}

		Box.HSplitBottom(60.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);

		Part.VMargin(60.0f, &Label);
		Label.VSplitLeft(100.0f, &Label, &TextBox);
		TextBox.VSplitLeft(20.0f, nullptr, &TextBox);
		Ui()->DoLabel(&Label, Localize("Name"), 18.0f, TEXTALIGN_ML);
		Ui()->DoClearableEditBox(&m_SkinNameInput, &TextBox, 12.0f);
	}
	else
	{
		Box.HSplitBottom(20.f, &Box, &Part);
		Box.HSplitBottom(24.f, &Box, &Part);
		Part.VMargin(120.0f, &Part);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
		{
			if(m_Popup == POPUP_DISCONNECTED && Client()->ReconnectTime() > 0)
				Client()->SetReconnectTime(0);
			m_Popup = POPUP_NONE;
		}
	}

	if(m_Popup == POPUP_NONE)
		Ui()->SetActiveItem(nullptr);
}

void CMenus::RenderPopupServerFull(CUIRect Screen)
{
	// Render #151515 background - use ACTUAL screen dimensions, not UI rect
	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0x15 / 255.0f, 0x15 / 255.0f, 0x15 / 255.0f, 1.0f);

	const int ScreenWidth = Graphics()->ScreenWidth();
	const int ScreenHeight = Graphics()->ScreenHeight();
	IGraphics::CQuadItem FullBg(0, 0, ScreenWidth, ScreenHeight);
	Graphics()->QuadsDrawTL(&FullBg, 1);
	Graphics()->QuadsEnd();

	// Load logo texture once when needed (lazy load)
	if(!m_TextureBCLogo.IsValid())
	{
		m_TextureBCLogo = Graphics()->LoadTexture("gui_logo.png", IStorage::TYPE_ALL);
	}

	CUIRect LogoArea;
	CUIRect ContentArea;
	const float LogoAreaHeight = minimum(150.0f, Screen.h * 0.28f);
	Screen.HSplitTop(LogoAreaHeight, &LogoArea, &ContentArea);

	if(m_TextureBCLogo.IsValid())
	{
		const float LogoWidth = 340.0f;
		const float LogoHeight = 97.0f;
		const float CenterX = LogoArea.x + LogoArea.w / 2.0f;
		const float CenterY = LogoArea.y + LogoArea.h / 2.0f;

		Graphics()->TextureSet(m_TextureBCLogo);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

		IGraphics::CQuadItem Logo(
			CenterX - LogoWidth / 2.0f,
			CenterY - LogoHeight / 2.0f,
			LogoWidth,
			LogoHeight);
		Graphics()->QuadsDrawTL(&Logo, 1);
		Graphics()->QuadsEnd();
	}

	float PanelW = minimum(520.0f, ContentArea.w - 40.0f);
	float PanelH = minimum(220.0f, ContentArea.h - 20.0f);
	const float MinW = minimum(220.0f, ContentArea.w);
	const float MinH = minimum(160.0f, ContentArea.h);
	PanelW = std::clamp(PanelW, MinW, ContentArea.w);
	PanelH = std::clamp(PanelH, MinH, ContentArea.h);
	CUIRect Panel;
	Panel.x = ContentArea.x + (ContentArea.w - PanelW) / 2.0f;
	Panel.y = ContentArea.y + (ContentArea.h - PanelH) / 2.0f;
	Panel.w = PanelW;
	Panel.h = PanelH;

	Panel.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 12.0f);
	Panel.DrawOutline(ColorRGBA(1.0f, 1.0f, 1.0f, 0.15f));

	const char *pTitle = Client()->ErrorString();
	const bool Reconnecting = Client()->ReconnectTime() > 0;
	char aExtra[128] = "";
	if(Reconnecting)
	{
		const int Seconds = (int)((Client()->ReconnectTime() - time_get()) / time_freq()) + 1;
		str_format(aExtra, sizeof(aExtra), Localize("Reconnect in %d sec"), Seconds);
	}
	else
	{
		str_copy(aExtra, Localize("Try again later."));
	}

	CUIRect Header;
	CUIRect Body;
	CUIRect ButtonRow;
	Panel.HSplitTop(34.0f, &Header, &Body);
	Body.HSplitBottom(40.0f, &Body, &ButtonRow);

	SLabelProperties TitleProps;
	TitleProps.m_MaxWidth = Header.w;
	Ui()->DoLabel(&Header, pTitle, 24.0f, TEXTALIGN_MC, TitleProps);

	SLabelProperties BodyProps;
	BodyProps.m_MaxWidth = Body.w;
	Ui()->DoLabel(&Body, aExtra, 18.0f, TEXTALIGN_MC, BodyProps);

	ButtonRow.VMargin(140.0f, &ButtonRow);
	static CButtonContainer s_Button;
	const char *pButtonText = Reconnecting ? Localize("Abort") : Localize("Ok");
	if(DoButton_Menu(&s_Button, pButtonText, 0, &ButtonRow) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER))
	{
		if(Reconnecting)
			Client()->SetReconnectTime(0);
		m_Popup = POPUP_NONE;
	}
}

void CMenus::RenderPopupConnecting(CUIRect Screen)
{
	(void)Screen;

	RenderPopupConnectionStatus(Localize("Connecting to"), Client()->ConnectAddressString());
}

void CMenus::RenderPopupConnectionStatus(const char *pStatusText, const char *pDetailText)
{
	CUIRect Screen = *Ui()->Screen();

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0x15 / 255.0f, 0x15 / 255.0f, 0x15 / 255.0f, 1.0f);

	IGraphics::CQuadItem FullBg(Screen.x, Screen.y, Screen.w, Screen.h);
	Graphics()->QuadsDrawTL(&FullBg, 1);
	Graphics()->QuadsEnd();

	if(!m_TextureBCLogo.IsValid())
	{
		m_TextureBCLogo = Graphics()->LoadTexture("gui_logo.png", IStorage::TYPE_ALL);
	}

	if(m_TextureBCLogo.IsValid())
	{
		const float LogoWidth = std::clamp(Screen.w * 0.43f, 330.0f, 620.0f);
		const float LogoHeight = LogoWidth * (103.0f / 360.0f);
		const float LogoX = Screen.x + (Screen.w - LogoWidth) * 0.5f;
		const float LogoY = Screen.y + Screen.h * 0.18f - LogoHeight * 0.5f;

		Graphics()->TextureSet(m_TextureBCLogo);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem Logo(LogoX, LogoY, LogoWidth, LogoHeight);
		Graphics()->QuadsDrawTL(&Logo, 1);
		Graphics()->QuadsEnd();
	}

	CUIRect TextArea;
	const float TextAreaWidth = Screen.w - 2.0f * minimum(100.0f, Screen.w * 0.08f);
	TextArea.w = TextAreaWidth;
	TextArea.h = 110.0f;
	TextArea.x = Screen.x + (Screen.w - TextArea.w) * 0.5f;
	TextArea.y = Screen.y + (Screen.h - TextArea.h) * 0.5f + 32.0f;

	const float StatusFontSize = std::clamp(Screen.h * 0.036f, 24.0f, 38.0f);
	const float DetailFontSize = std::clamp(StatusFontSize * 0.86f, 20.0f, 32.0f);

	CUIRect Label;
	SLabelProperties Props;
	Props.m_MaxWidth = TextArea.w;
	Props.m_EllipsisAtEnd = true;

	TextArea.HSplitTop(StatusFontSize + 4.0f, &Label, &TextArea);
	Ui()->DoLabel(&Label, pStatusText ? pStatusText : "", StatusFontSize, TEXTALIGN_MC, Props);

	TextArea.HSplitTop(16.0f, nullptr, &TextArea);
	TextArea.HSplitTop(DetailFontSize + 4.0f, &Label, &TextArea);
	Ui()->DoLabel(&Label, pDetailText ? pDetailText : "", DetailFontSize, TEXTALIGN_MC, Props);

	CUIRect Button;
	Screen.HSplitBottom(46.0f, nullptr, &Button);
	Button.VMargin(14.0f, &Button);
	Button.h = 32.0f;

	static CButtonContainer s_CancelButton;
	const bool Cancel = DoButton_Menu(
		&s_CancelButton,
		Localize("Cancel"),
		0,
		&Button,
		BUTTONFLAG_LEFT,
		nullptr,
		IGraphics::CORNER_ALL,
		10.0f,
		0.15f,
		ColorRGBA(0.55f, 0.55f, 0.55f, 1.0f)) ||
		Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE);

	if(Cancel)
	{
		Client()->Disconnect();
		Ui()->SetActiveItem(nullptr);
		RefreshBrowserTab(true);
	}
}


void CMenus::RenderPopupLoading(CUIRect Screen)
{
	(void)Screen;

	char aStatus[128];
	char aDetail[256];
	aStatus[0] = '\0';
	aDetail[0] = '\0';

	switch(Client()->LoadingStateDetail())
	{
	case IClient::LOADING_STATE_DETAIL_LOADING_MAP:
	{
		const int TotalSize = Client()->MapDownloadTotalsize();
		const int CurrentSize = Client()->MapDownloadAmount();
		if(TotalSize > 0 && CurrentSize >= 0 && CurrentSize < TotalSize)
		{
			str_copy(aStatus, Localize("Downloading map"), sizeof(aStatus));
			const int Percent = std::clamp((CurrentSize * 100) / TotalSize, 0, 100);
			const char *pMapName = Client()->MapDownloadName();
			if(pMapName != nullptr && pMapName[0] != '\0')
				str_format(aDetail, sizeof(aDetail), "%s (%d%%)", pMapName, Percent);
			else
				str_format(aDetail, sizeof(aDetail), Localize("%d%%"), Percent);
		}
		else
		{
			str_copy(aStatus, Localize("Loading map"), sizeof(aStatus));
		}
		break;
	}
	case IClient::LOADING_STATE_DETAIL_LOADING_DEMO:
		str_copy(aStatus, Localize("Loading demo file"), sizeof(aStatus));
		break;
	case IClient::LOADING_STATE_DETAIL_SENDING_READY:
		str_copy(aStatus, Localize("Sending ready state"), sizeof(aStatus));
		break;
	case IClient::LOADING_STATE_DETAIL_GETTING_READY:
		str_copy(aStatus, Localize("Preparing game client"), sizeof(aStatus));
		break;
	case IClient::LOADING_STATE_DETAIL_INITIAL:
	default:
		str_copy(aStatus, Localize("Waiting for server data"), sizeof(aStatus));
		break;
	}

	if(aDetail[0] == '\0')
		str_copy(aDetail, Client()->ConnectAddressString(), sizeof(aDetail));

	RenderPopupConnectionStatus(aStatus, aDetail);
}


#if defined(CONF_VIDEORECORDER)
void CMenus::PopupConfirmDemoReplaceVideo()
{
	char aBuf[IO_MAX_PATH_LENGTH];
	str_format(aBuf, sizeof(aBuf), "%s/%s.demo", m_aCurrentDemoFolder, m_aCurrentDemoSelectionName);
	char aVideoName[IO_MAX_PATH_LENGTH];
	str_copy(aVideoName, m_DemoRenderInput.GetString());
	const char *pError = Client()->DemoPlayer_Render(aBuf, m_DemolistStorageType, aVideoName, m_Speed, m_StartPaused);
	m_Speed = DEMO_SPEED_INDEX_DEFAULT;
	m_StartPaused = false;
	m_LastPauseChange = -1.0f;
	m_LastSpeedChange = -1.0f;
	if(pError)
	{
		m_DemoRenderInput.Clear();
		PopupMessage(Localize("Error loading demo"), pError, Localize("Ok"));
	}
}
#endif

void CMenus::RenderThemeSelection(CUIRect MainView)
{
	static constexpr float s_RowHeight = 34.0f;

	const std::vector<CTheme> &vThemes = GameClient()->m_MenuBackground.GetThemes();

	int SelectedTheme = -1;
	for(int i = 0; i < (int)vThemes.size(); i++)
	{
		if(str_comp(vThemes[i].m_Name.c_str(), g_Config.m_ClMenuMap) == 0)
		{
			SelectedTheme = i;
			break;
		}
	}
	const int OldSelected = SelectedTheme;

	static CListBox s_ListBox;
	s_ListBox.DoHeader(&MainView, Localize("Theme"), 20.0f);
	s_ListBox.DoStart(s_RowHeight, vThemes.size(), 1, 4, SelectedTheme);

	for(int i = 0; i < (int)vThemes.size(); i++)
	{
		const CTheme &Theme = vThemes[i];
		const CListboxItem Item = s_ListBox.DoNextItem(&Theme.m_Name, i == SelectedTheme);

		if(!Item.m_Visible)
			continue;

		CUIRect Icon, Label;
		const float Aspect = maximum(Theme.m_IconAspect, 1.0f / 8.0f);
		Item.m_Rect.VSplitLeft(s_RowHeight * 2.75f, &Icon, &Label);

		// draw icon if it exists
		if(Theme.m_IconTexture.IsValid())
		{
			Icon.Margin(4.0f, &Icon);
			if(Icon.w / Icon.h > Aspect)
			{
				const float Width = Icon.h * Aspect;
				Icon.x += (Icon.w - Width) / 2.0f;
				Icon.w = Width;
			}
			else
			{
				const float Height = Icon.w / Aspect;
				Icon.y += (Icon.h - Height) / 2.0f;
				Icon.h = Height;
			}
			Graphics()->TextureSet(Theme.m_IconTexture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		Label.VMargin(4.0f, &Label);

		char aName[128];
		if(Theme.m_Name.empty())
			str_copy(aName, "(none)");
		else if(str_comp(Theme.m_Name.c_str(), "auto") == 0)
			str_copy(aName, "(seasons)");
		else if(str_comp(Theme.m_Name.c_str(), "rand") == 0)
			str_copy(aName, "(random)");
		else if(Theme.m_HasDay && Theme.m_HasNight)
			str_copy(aName, Theme.m_Name.c_str());
		else if(Theme.m_HasDay && !Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s (day)", Theme.m_Name.c_str());
		else if(!Theme.m_HasDay && Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s (night)", Theme.m_Name.c_str());
		else // generic
			str_copy(aName, Theme.m_Name.c_str());

		Ui()->DoLabel(&Label, aName, 15.0f * CUi::ms_FontmodHeight, TEXTALIGN_ML);
	}

	SelectedTheme = s_ListBox.DoEnd();

	if(OldSelected != SelectedTheme)
	{
		const CTheme &Theme = vThemes[SelectedTheme];
		str_copy(g_Config.m_ClMenuMap, Theme.m_Name.c_str());
		GameClient()->m_MenuBackground.LoadMenuBackground(Theme.m_HasDay, Theme.m_HasNight);
	}
}

void CMenus::SetActive(bool Active)
{
	if(Active != m_MenuActive)
	{
		Ui()->SetHotItem(nullptr);
		Ui()->SetActiveItem(nullptr);
	}
	m_MenuActive = Active;
	if(!m_MenuActive)
	{
		m_BcIngameMenuClosing = false;
		m_BcIngameMenuOpenPhase = 0.0f;
		if(m_NeedSendinfo)
		{
			GameClient()->SendInfo(false);
			m_NeedSendinfo = false;
		}

		if(m_NeedSendDummyinfo)
		{
			GameClient()->SendDummyInfo(false);
			m_NeedSendDummyinfo = false;
		}

		if(Client()->State() == IClient::STATE_ONLINE)
		{
			GameClient()->OnRelease();
		}
	}
	else if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		GameClient()->OnRelease();
	}
	else if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		// BestClient: ingame ESC menu open animation starts from 0.
		m_BcIngameMenuClosing = false;
		m_BcIngameMenuOpenPhase = (BCUiAnimations::Enabled() && g_Config.m_BcIngameMenuAnimation != 0 && g_Config.m_BcIngameMenuAnimationMs > 0) ? 0.0f : 1.0f;
	}
	else
	{
		m_BcIngameMenuClosing = false;
		m_BcIngameMenuOpenPhase = 1.0f;
	}
}

void CMenus::OnReset()
{
	m_MenuMediaBackground.Unload();
}

void CMenus::OnShutdown()
{
	m_MenuMediaBackground.Shutdown();
	Ui()->ClearButtonSoundEventCallback();
	UnloadMenuSfx();
	m_CommunityIcons.Shutdown();
}

bool CMenus::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!m_MenuActive)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	Ui()->OnCursorMove(x, y);

	return true;
}

bool CMenus::OnInput(const IInput::CEvent &Event)
{
	// Escape key is always handled to activate/deactivate menu
	if((Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE) || IsActive())
	{
		// BestClient: smooth close animation for ingame ESC menu.
		if((Event.m_Flags & IInput::FLAG_PRESS) && Event.m_Key == KEY_ESCAPE &&
			IsActive() && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK) &&
			BCUiAnimations::Enabled() && g_Config.m_BcIngameMenuAnimation != 0 && g_Config.m_BcIngameMenuAnimationMs > 0)
		{
			m_BcIngameMenuClosing = true;
			return true;
		}

		auto IsSettingsSearchTriggerKey = [](int Key) {
			return (Key >= KEY_A && Key <= KEY_Z) ||
				(Key >= KEY_1 && Key <= KEY_0) ||
				(Key >= KEY_SPACE && Key <= KEY_SLASH) ||
				(Key >= KEY_KP_0 && Key <= KEY_KP_PERIOD) ||
				Key == KEY_KP_PLUS || Key == KEY_KP_MINUS || Key == KEY_KP_MULTIPLY || Key == KEY_KP_DIVIDE;
		};

		const bool InSettingsPage =
			(Client()->State() == IClient::STATE_ONLINE && m_GamePage == PAGE_SETTINGS) ||
			(Client()->State() != IClient::STATE_ONLINE && m_MenuPage == PAGE_SETTINGS);
		if(IsBestClientFunTabActive() && m_SettingsSearchOpen)
			ResetSettingsSearch();
		const bool HasTextInputEvent = (Event.m_Flags & IInput::FLAG_TEXT) && Event.m_aText[0] != '\0';
		const bool HasPrintableKeyPress =
			(Event.m_Flags & IInput::FLAG_PRESS) && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() &&
			IsSettingsSearchTriggerKey(Event.m_Key);

		// Start settings search immediately when typing, but never steal input
		// from other edit boxes, popups or active key binding.
		if((HasTextInputEvent || HasPrintableKeyPress) &&
			InSettingsPage && !IsBestClientFunTabActive() &&
			m_Popup == POPUP_NONE && !Ui()->IsPopupOpen() &&
			!GameClient()->m_KeyBinder.IsActive() && CLineInput::GetActiveInput() == nullptr)
		{
			OpenSettingsSearch();
		}

		Ui()->OnInput(Event);
		return true;
	}
	return false;
}

void CMenus::OnStateChange(int NewState, int OldState)
{
	// reset active item
	Ui()->SetActiveItem(nullptr);

	if(OldState == IClient::STATE_ONLINE || OldState == IClient::STATE_OFFLINE)
		TextRender()->DeleteTextContainer(m_MotdTextContainerIndex);

	if(NewState == IClient::STATE_OFFLINE)
	{
		if(OldState >= IClient::STATE_ONLINE && NewState < IClient::STATE_QUITTING)
			UpdateMusicState();
		m_Popup = POPUP_NONE;
		if(Client()->ErrorString() && Client()->ErrorString()[0] != 0)
		{
			if(str_find(Client()->ErrorString(), "password"))
			{
				m_Popup = POPUP_PASSWORD;
				m_PasswordInput.SelectAll();
				Ui()->SetActiveItem(&m_PasswordInput);
			}
			else
				m_Popup = POPUP_DISCONNECTED;
		}
	}
	else if(NewState == IClient::STATE_LOADING)
	{
		m_DownloadLastCheckTime = time_get();
		m_DownloadLastCheckSize = 0;
		m_DownloadSpeed = 0.0f;
	}
	else if(NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK)
	{
		if(m_Popup != POPUP_WARNING)
		{
			m_Popup = POPUP_NONE;
			SetActive(false);
		}
	}
}

void CMenus::OnWindowResize()
{
	TextRender()->DeleteTextContainer(m_MotdTextContainerIndex);
}

void CMenus::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		SetActive(true);

	if(Client()->State() == IClient::STATE_ONLINE && GameClient()->m_ServerMode == CGameClient::SERVERMODE_PUREMOD)
	{
		Client()->Disconnect();
		SetActive(true);
		PopupMessage(Localize("Disconnected"), Localize("The server is running a non-standard tuning on a pure game type."), Localize("Ok"));
	}

	if(!IsActive())
	{
		if(Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		{
			SetActive(true);
		}
		else if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		{
			Ui()->ClearHotkeys();
			return;
		}
	}

	Ui()->StartCheck();
	UpdateColors();

	// BestClient: disable interaction while ESC menu is animating (open/close) to avoid buggy clicks/hover.
	const bool IngameMenu = IsActive() && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK);
	const bool IngameMenuAnimated = IngameMenu && BCUiAnimations::Enabled() && g_Config.m_BcIngameMenuAnimation != 0 && g_Config.m_BcIngameMenuAnimationMs > 0;
	if(IngameMenuAnimated)
	{
		const bool AllowInput = !m_BcIngameMenuClosing && m_BcIngameMenuOpenPhase >= 0.999f;
		Ui()->SetEnabled(AllowInput);
		if(!AllowInput)
		{
			Ui()->SetHotItem(nullptr);
			Ui()->SetActiveItem(nullptr);
		}
	}
	else
	{
		Ui()->SetEnabled(true);
	}

	Ui()->Update();

	// BestClient: animate ESC menu open/close only ingame.
	if(IngameMenu)
	{
		if(BCUiAnimations::Enabled() && g_Config.m_BcIngameMenuAnimation != 0 && g_Config.m_BcIngameMenuAnimationMs > 0)
		{
			const float Target = m_BcIngameMenuClosing ? 0.0f : 1.0f;
			BCUiAnimations::UpdatePhase(m_BcIngameMenuOpenPhase, Target, Client()->RenderFrameTime(), BCUiAnimations::MsToSeconds(g_Config.m_BcIngameMenuAnimationMs));
			if(m_BcIngameMenuClosing && m_BcIngameMenuOpenPhase <= 0.0f)
			{
				Ui()->SetEnabled(true);
				SetActive(false);
				Ui()->FinishCheck();
				Ui()->ClearHotkeys();
				return;
			}
		}
		else
			m_BcIngameMenuOpenPhase = 1.0f;
	}

	Render();

	if(IsActive())
	{
		RenderTools()->RenderCursor(Ui()->MousePos(), 24.0f);
	}

	// render debug information
	if(g_Config.m_Debug)
		Ui()->DebugRender(2.0f, Ui()->Screen()->h - 12.0f);

	if(Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
	{
		Ui()->SetHotItem(nullptr);
		Ui()->SetActiveItem(nullptr);
		if(IngameMenuAnimated)
			m_BcIngameMenuClosing = true;
		else
			SetActive(false);
	}

	Ui()->FinishCheck();
	Ui()->ClearHotkeys();
}

void CMenus::UpdateColors()
{
	ms_GuiColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_UiColor, true));

	ms_ColorTabbarInactiveOutgame = ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f);
	ms_ColorTabbarActiveOutgame = ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f);
	ms_ColorTabbarHoverOutgame = ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f);

	const float ColorIngameScaleI = 0.5f;
	const float ColorIngameScaleA = 0.2f;

	ms_ColorTabbarInactiveIngame = ColorRGBA(
		ms_GuiColor.r * ColorIngameScaleI,
		ms_GuiColor.g * ColorIngameScaleI,
		ms_GuiColor.b * ColorIngameScaleI,
		ms_GuiColor.a * 0.8f);

	ms_ColorTabbarActiveIngame = ColorRGBA(
		ms_GuiColor.r * ColorIngameScaleA,
		ms_GuiColor.g * ColorIngameScaleA,
		ms_GuiColor.b * ColorIngameScaleA,
		ms_GuiColor.a);

	ms_ColorTabbarHoverIngame = ColorRGBA(1.0f, 1.0f, 1.0f, 0.75f);
}

void CMenus::RenderBackground()
{
	Graphics()->BlendNormal();

	const float ScreenHeight = 300.0f;
	const float ScreenWidth = ScreenHeight * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0.0f, 0.0f, ScreenWidth, ScreenHeight);

	// render background color
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(ms_GuiColor.WithAlpha(1.0f));
	const IGraphics::CQuadItem BackgroundQuadItem = IGraphics::CQuadItem(0, 0, ScreenWidth, ScreenHeight);
	Graphics()->QuadsDrawTL(&BackgroundQuadItem, 1);
	Graphics()->QuadsEnd();

	// render the tiles
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.045f);
	const float Size = 15.0f;
	const float OffsetTime = std::fmod(Client()->GlobalTime() * 0.15f, 2.0f);
	IGraphics::CQuadItem aCheckerItems[64];
	size_t NumCheckerItems = 0;
	const int NumItemsWidth = std::ceil(ScreenWidth / Size);
	const int NumItemsHeight = std::ceil(ScreenHeight / Size);
	for(int y = -2; y < NumItemsHeight; y++)
	{
		for(int x = 0; x < NumItemsWidth + 4; x += 2)
		{
			aCheckerItems[NumCheckerItems] = IGraphics::CQuadItem((x - 2 * OffsetTime + (y & 1)) * Size, (y + OffsetTime) * Size, Size, Size);
			NumCheckerItems++;
			if(NumCheckerItems == std::size(aCheckerItems))
			{
				Graphics()->QuadsDrawTL(aCheckerItems, NumCheckerItems);
				NumCheckerItems = 0;
			}
		}
	}
	if(NumCheckerItems != 0)
		Graphics()->QuadsDrawTL(aCheckerItems, NumCheckerItems);
	Graphics()->QuadsEnd();

	// render border fade
	Graphics()->TextureSet(m_TextureBlob);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	const IGraphics::CQuadItem BlobQuadItem = IGraphics::CQuadItem(-100, -100, ScreenWidth + 200, ScreenHeight + 200);
	Graphics()->QuadsDrawTL(&BlobQuadItem, 1);
	Graphics()->QuadsEnd();

	// restore screen
	Ui()->MapScreen();
}

int CMenus::DoButton_CheckBox_Tristate(const void *pId, const char *pText, TRISTATE Checked, const CUIRect *pRect)
{
	switch(Checked)
	{
	case TRISTATE::NONE:
		return DoButton_CheckBox_Common(pId, pText, "", pRect, BUTTONFLAG_LEFT);
	case TRISTATE::SOME:
		return DoButton_CheckBox_Common(pId, pText, "O", pRect, BUTTONFLAG_LEFT);
	case TRISTATE::ALL:
		return DoButton_CheckBox_Common(pId, pText, "X", pRect, BUTTONFLAG_LEFT);
	default:
		dbg_assert_failed("Invalid tristate. Checked: %d", static_cast<int>(Checked));
	}
}

int CMenus::MenuImageScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	const char *pExtension = ".png";
	CMenuImage MenuImage;
	CMenus *pSelf = static_cast<CMenus *>(pUser);
	if(IsDir || !str_endswith(pName, pExtension) || str_length(pName) - str_length(pExtension) >= (int)sizeof(MenuImage.m_aName))
		return 0;

	char aPath[IO_MAX_PATH_LENGTH];
	str_format(aPath, sizeof(aPath), "menuimages/%s", pName);

	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPng(Info, aPath, DirType))
	{
		char aError[IO_MAX_PATH_LENGTH + 64];
		str_format(aError, sizeof(aError), "Failed to load menu image from '%s'", aPath);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "menus", aError);
		return 0;
	}
	if(Info.m_Format != CImageInfo::FORMAT_RGBA)
	{
		Info.Free();
		char aError[IO_MAX_PATH_LENGTH + 64];
		str_format(aError, sizeof(aError), "Failed to load menu image from '%s': must be an RGBA image", aPath);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "menus", aError);
		return 0;
	}

	MenuImage.m_OrgTexture = pSelf->Graphics()->LoadTextureRaw(Info, 0, aPath);

	ConvertToGrayscale(Info);
	MenuImage.m_GreyTexture = pSelf->Graphics()->LoadTextureRawMove(Info, 0, aPath);

	str_truncate(MenuImage.m_aName, sizeof(MenuImage.m_aName), pName, str_length(pName) - str_length(pExtension));
	pSelf->m_vMenuImages.push_back(MenuImage);

	pSelf->RenderLoading(Localize("Loading DDNet Client"), Localize("Loading menu images"), 0);

	return 0;
}

const CMenus::CMenuImage *CMenus::FindMenuImage(const char *pName)
{
	for(auto &Image : m_vMenuImages)
		if(str_comp(Image.m_aName, pName) == 0)
			return &Image;
	return nullptr;
}

void CMenus::SetMenuPage(int NewPage)
{
	const int OldPage = m_MenuPage;
	m_MenuPage = NewPage;
	if(NewPage >= PAGE_INTERNET && NewPage <= PAGE_FAVORITE_COMMUNITY_5)
	{
		g_Config.m_UiPage = NewPage;
		bool ForceRefresh = false;
		if(m_ForceRefreshLanPage)
		{
			ForceRefresh = NewPage == PAGE_LAN;
			m_ForceRefreshLanPage = false;
		}
		if(OldPage != NewPage || ForceRefresh)
		{
			RefreshBrowserTab(ForceRefresh);
		}
	}
}

void CMenus::RefreshBrowserTab(bool Force)
{
	GameClient()->m_ClientIndicator.RefreshBrowserCache(Force);

	if(g_Config.m_UiPage == PAGE_INTERNET)
	{
		if(Force || ServerBrowser()->GetCurrentType() != IServerBrowser::TYPE_INTERNET)
		{
			if(Force || ServerBrowser()->GetCurrentType() == IServerBrowser::TYPE_LAN)
			{
				Client()->RequestDDNetInfo();
			}
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			UpdateCommunityCache(true);
		}
	}
	else if(g_Config.m_UiPage == PAGE_LAN)
	{
		if(Force || ServerBrowser()->GetCurrentType() != IServerBrowser::TYPE_LAN)
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			UpdateCommunityCache(true);
		}
	}
	else if(g_Config.m_UiPage == PAGE_FAVORITES)
	{
		if(Force || ServerBrowser()->GetCurrentType() != IServerBrowser::TYPE_FAVORITES)
		{
			if(Force || ServerBrowser()->GetCurrentType() == IServerBrowser::TYPE_LAN)
			{
				Client()->RequestDDNetInfo();
			}
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			UpdateCommunityCache(true);
		}
	}
	else if(g_Config.m_UiPage >= PAGE_FAVORITE_COMMUNITY_1 && g_Config.m_UiPage <= PAGE_FAVORITE_COMMUNITY_5)
	{
		const int BrowserType = g_Config.m_UiPage - PAGE_FAVORITE_COMMUNITY_1 + IServerBrowser::TYPE_FAVORITE_COMMUNITY_1;
		if(Force || ServerBrowser()->GetCurrentType() != BrowserType)
		{
			if(Force || ServerBrowser()->GetCurrentType() == IServerBrowser::TYPE_LAN)
			{
				Client()->RequestDDNetInfo();
			}
			ServerBrowser()->Refresh(BrowserType);
			UpdateCommunityCache(true);
		}
	}
}

void CMenus::ForceRefreshLanPage()
{
	m_ForceRefreshLanPage = true;
}

void CMenus::SetShowStart(bool ShowStart)
{
	m_ShowStart = ShowStart;
}

void CMenus::ShowQuitPopup()
{
	m_Popup = POPUP_QUIT;
}

void CMenus::JoinTutorial()
{
	m_JoinTutorial = true;
}
