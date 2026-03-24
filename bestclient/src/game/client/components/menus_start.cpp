/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus_start.h"

#include <engine/client/updater.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>
#include <game/version.h>

#if defined(CONF_PLATFORM_ANDROID)
#include <android/android_main.h>
#endif

void CMenusStart::RenderStartMenu(CUIRect MainView)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_START);

	// render logo
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	IGraphics::CQuadItem QuadItem(MainView.w / 2 - 170, 60, 360, 103);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	const float Rounding = 10.0f;
	const float VMargin = MainView.w / 2 - 190.0f;
	const float ExtMenuBottomOffset = 40.0f;

	CUIRect Button;
	int NewPage = -1;
	const auto &&SetIconMode = [&](bool Enable) {
		if(Enable)
		{
			TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
			TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		}
		else
		{
			TextRender()->SetRenderFlags(0);
			TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		}
	};

	CUIRect ExtMenu;
	MainView.VSplitLeft(30.0f, nullptr, &ExtMenu);
	ExtMenu.VSplitLeft(100.0f, &ExtMenu, nullptr);
	ExtMenu.HSplitBottom(ExtMenuBottomOffset, &ExtMenu, nullptr);

	ExtMenu.HSplitBottom(20.0f, &ExtMenu, &Button);
	static CButtonContainer s_DiscordButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_DiscordButton, Localize("Discord"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Client()->ViewLink("https://discord.gg/tmT4emAbrS");
	}

	ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr); // little space
	ExtMenu.HSplitBottom(20.0f, &ExtMenu, &Button);
	static CButtonContainer s_TelegramButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_TelegramButton, Localize("Telegram"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Client()->ViewLink("https://t.me/bestddnet");
	}

	ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr); // little space
	ExtMenu.HSplitBottom(20.0f, &ExtMenu, &Button);
	static CButtonContainer s_CheckUpdateButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_CheckUpdateButton, Localize("Check update"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		GameClient()->m_BestClient.FetchBestClientInfo();
	}

	CUIRect Menu;
	MainView.VMargin(VMargin, &Menu);
	Menu.HSplitBottom(25.0f, &Menu, nullptr);

	Menu.HSplitBottom(40.0f, &Menu, &Button);
	CUIRect QuitButton = Button;
	QuitButton.w = QuitButton.h;
	QuitButton.x += (Button.w - QuitButton.w) / 2.0f;
	static CButtonContainer s_QuitButton;
	bool UsedEscape = false;
	SetIconMode(true);
	if(GameClient()->m_Menus.DoButton_Menu(&s_QuitButton, FontIcons::FONT_ICON_POWER_OFF, 0, &QuitButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, Rounding, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || (UsedEscape = Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE)) || CheckHotKey(KEY_Q))
	{
		SetIconMode(false);
		if(UsedEscape || GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
		{
			GameClient()->m_Menus.ShowQuitPopup();
		}
		else
		{
			Client()->Quit();
		}
	}
	SetIconMode(false);

	Menu.HSplitBottom(100.0f, &Menu, nullptr);

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	const CUIRect SettingsButton = Button;
	static CButtonContainer s_SettingsButton;
	if(GameClient()->m_Menus.DoButton_MenuEx(&s_SettingsButton, Localize("Settings"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "settings" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), g_Config.m_ClShowStartMenuImages) || CheckHotKey(KEY_S))
		NewPage = CMenus::PAGE_SETTINGS;

#if defined(CONF_AUTOUPDATE)
	char aUpdateBuf[128] = "";
	const bool NeedUpdate = GameClient()->m_BestClient.NeedUpdate();
	const IUpdater::EUpdaterState State = Updater()->GetCurrentState();
	const bool ShowDownloadButton = NeedUpdate && State == IUpdater::CLEAN;
	const bool ShowRetryButton = NeedUpdate && State == IUpdater::FAIL;
	const bool ShowUpdateButton = State == IUpdater::NEED_RESTART;
	const bool ShowUpdateProgress = State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART;

	if(ShowDownloadButton || ShowRetryButton || ShowUpdateButton || ShowUpdateProgress)
	{
		CUIRect UpdateRow = SettingsButton;
		UpdateRow.y += SettingsButton.h + 5.0f;
		UpdateRow.h = 22.0f;

		CUIRect UpdateLabel, UpdateButton;
		UpdateRow.VSplitRight(120.0f, &UpdateLabel, &UpdateButton);
		UpdateLabel.VSplitRight(10.0f, &UpdateLabel, nullptr);

		if(ShowDownloadButton)
		{
			str_format(aUpdateBuf, sizeof(aUpdateBuf), "BestClient %s Is release", GameClient()->m_BestClient.m_aVersionStr);
		}
		else if(ShowUpdateProgress)
		{
			if(State == IUpdater::GETTING_MANIFEST)
				str_copy(aUpdateBuf, Localize("Preparing update..."));
			else
				str_format(aUpdateBuf, sizeof(aUpdateBuf), Localize("Downloading %d%%"), Updater()->GetCurrentPercent());
		}
		else if(State == IUpdater::FAIL)
		{
			str_copy(aUpdateBuf, Localize("Update failed"));
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		}
		else if(State == IUpdater::NEED_RESTART)
		{
			str_copy(aUpdateBuf, Localize("Update downloaded"));
			TextRender()->TextColor(0.7f, 1.0f, 0.7f, 1.0f);
		}

		if(ShowDownloadButton)
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);

		Ui()->DoLabel(&UpdateLabel, aUpdateBuf, 14.0f, TEXTALIGN_ML);
		TextRender()->TextColor(TextRender()->DefaultTextColor());

		if(ShowDownloadButton || ShowRetryButton)
		{
			static CButtonContainer s_MenuUpdateDownload;
			if(GameClient()->m_Menus.DoButton_Menu(&s_MenuUpdateDownload, Localize("Download"), 0, &UpdateButton, BUTTONFLAG_LEFT, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
			{
				Updater()->InitiateUpdate();
			}
		}
		else if(ShowUpdateButton)
		{
			static CButtonContainer s_MenuUpdateRestart;
			if(GameClient()->m_Menus.DoButton_Menu(&s_MenuUpdateRestart, Localize("Restart"), 0, &UpdateButton, BUTTONFLAG_LEFT, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
			{
				Updater()->ApplyUpdateAndRestart();
			}
		}
		else
		{
			Ui()->RenderProgressBar(UpdateButton, Updater()->GetCurrentPercent() / 100.0f);
		}
	}
#endif

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_LocalServerButton;

	const bool LocalServerRunning = GameClient()->m_LocalServer.IsServerRunning();
	if(GameClient()->m_Menus.DoButton_MenuEx(&s_LocalServerButton, LocalServerRunning ? Localize("Stop server") : Localize("Run server"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "local_server" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, LocalServerRunning ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), g_Config.m_ClShowStartMenuImages) || (CheckHotKey(KEY_R) && Input()->KeyPress(KEY_R)))
	{
		if(LocalServerRunning)
		{
			GameClient()->m_LocalServer.KillServer();
		}
		else
		{
			GameClient()->m_LocalServer.RunServer({});
		}
	}

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_MapEditorButton;
	if(GameClient()->m_Menus.DoButton_MenuEx(&s_MapEditorButton, Localize("Editor"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "editor" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, GameClient()->Editor()->HasUnsavedData() ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), g_Config.m_ClShowStartMenuImages) || CheckHotKey(KEY_E))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_DemoButton;
	if(GameClient()->m_Menus.DoButton_MenuEx(&s_DemoButton, Localize("Demos"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "demos" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), g_Config.m_ClShowStartMenuImages) || CheckHotKey(KEY_D))
	{
		NewPage = CMenus::PAGE_DEMOS;
	}

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_PlayButton;
	if(GameClient()->m_Menus.DoButton_MenuEx(&s_PlayButton, Localize("Play", "Start menu"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "play_game" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), g_Config.m_ClShowStartMenuImages) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
	{
		NewPage = g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5 ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;
	}

	// render version
	CUIRect CurVersion, ConsoleButton;
	MainView.HSplitBottom(60.0f, nullptr, &CurVersion);
	CurVersion.VSplitRight(40.0f, &CurVersion, nullptr);
	CurVersion.HSplitTop(20.0f, &ConsoleButton, &CurVersion);
	CurVersion.HSplitTop(5.0f, nullptr, &CurVersion);
	ConsoleButton.VSplitRight(40.0f, nullptr, &ConsoleButton);
	CUIRect VersionLine1, VersionLine2;
	CurVersion.HSplitTop(16.0f, &VersionLine1, &CurVersion);
	CurVersion.HSplitTop(16.0f, &VersionLine2, &CurVersion);

	char aDDNetBuf[64];
	char aBestBuf[64];
	str_format(aDDNetBuf, sizeof(aDDNetBuf), "DDNet %s", GAME_RELEASE_VERSION);
	str_format(aBestBuf, sizeof(aBestBuf), "BestClient %s", BestClient_VERSION);
	Ui()->DoLabel(&VersionLine1, aDDNetBuf, 14.0f, TEXTALIGN_MR);
	Ui()->DoLabel(&VersionLine2, aBestBuf, 14.0f, TEXTALIGN_MR);

	static CButtonContainer s_ConsoleButton;
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	if(GameClient()->m_Menus.DoButton_Menu(&s_ConsoleButton, FontIcons::FONT_ICON_TERMINAL, 0, &ConsoleButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f)))
	{
		GameClient()->m_GameConsole.Toggle(CGameConsole::CONSOLETYPE_LOCAL);
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

bool CMenusStart::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() && // no modifier
	       Input()->KeyPress(Key) &&
	       !GameClient()->m_GameConsole.IsActive();
}
