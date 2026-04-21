/* Copyright © 2026 BestProject Team */
#include "cef_runtime.h"

#include <base/log.h>
#include <base/math.h>
#include <base/str.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/storage.h>

#if (defined(CONF_FAMILY_WINDOWS) || defined(CONF_PLATFORM_LINUX)) && defined(CONF_BESTCLIENT_CEF)

#include <base/system.h>

#if defined(CONF_FAMILY_WINDOWS)
#include <base/windows.h>
#endif

#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_command_line.h>
#include <include/cef_life_span_handler.h>
#if defined(CONF_FAMILY_WINDOWS)
#include <include/cef_sandbox_win.h>
#endif

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#if defined(CONF_PLATFORM_LINUX)
#include <limits.h>
#include <unistd.h>
#include <X11/Xlib.h>
#ifdef Status
#undef Status
#endif
#endif

namespace
{
class CBestClientCefApp final : public CefApp
{
	IMPLEMENT_REFCOUNTING(CBestClientCefApp);
};

#if defined(CONF_PLATFORM_LINUX)
int g_CefArgc = 0;
const char **g_pCefArgv = nullptr;
#endif

std::filesystem::path GetModulePath()
{
#if defined(CONF_FAMILY_WINDOWS)
	wchar_t aPath[MAX_PATH] = {};
	const DWORD Length = GetModuleFileNameW(nullptr, aPath, sizeof(aPath) / sizeof(aPath[0]));
	if(Length == 0 || Length >= sizeof(aPath) / sizeof(aPath[0]))
		return {};
	return std::filesystem::path(aPath);
#else
	char aPath[PATH_MAX] = {};
	const ssize_t Length = readlink("/proc/self/exe", aPath, sizeof(aPath) - 1);
	if(Length <= 0 || static_cast<size_t>(Length) >= sizeof(aPath))
		return {};
	aPath[Length] = '\0';
	return std::filesystem::path(aPath);
#endif
}

std::filesystem::path GetModuleDirectory()
{
	const std::filesystem::path ModulePath = GetModulePath();
	return ModulePath.empty() ? std::filesystem::path() : ModulePath.parent_path();
}

CefMainArgs MakeCefMainArgs()
{
#if defined(CONF_FAMILY_WINDOWS)
	return CefMainArgs(GetModuleHandleW(nullptr));
#else
	return CefMainArgs(g_CefArgc, const_cast<char **>(g_pCefArgv));
#endif
}

CefWindowHandle NativeWindowHandleFromPointer(void *pHandle)
{
#if defined(CONF_FAMILY_WINDOWS)
	return static_cast<CefWindowHandle>(pHandle);
#else
	return static_cast<CefWindowHandle>(reinterpret_cast<uintptr_t>(pHandle));
#endif
}

bool IsValidWindowHandle(CefWindowHandle WindowHandle)
{
#if defined(CONF_FAMILY_WINDOWS)
	return WindowHandle != nullptr;
#else
	return WindowHandle != 0;
#endif
}

void ClearWindowHandle(CefWindowHandle &WindowHandle)
{
#if defined(CONF_FAMILY_WINDOWS)
	WindowHandle = nullptr;
#else
	WindowHandle = 0;
#endif
}

void SetCefPath(cef_string_t &Target, const std::filesystem::path &Path)
{
	CefString TargetString(&Target);
#if defined(CONF_FAMILY_WINDOWS)
	TargetString = Path.native();
#else
	TargetString = Path.string();
#endif
}

void SetCefUtf8Path(cef_string_t &Target, const char *pPath)
{
	CefString TargetString(&Target);
#if defined(CONF_FAMILY_WINDOWS)
	TargetString = windows_utf8_to_wide(pPath);
#else
	TargetString = pPath;
#endif
}
}

class CBestClientBrowserImpl;

class CBestClientBrowserHandler final : public CefClient, public CefLifeSpanHandler
{
public:
	explicit CBestClientBrowserHandler(CBestClientBrowserImpl *pOwner) :
		m_pOwner(pOwner)
	{
	}

	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
	void OnAfterCreated(CefRefPtr<CefBrowser> pBrowser) override;
	void OnBeforeClose(CefRefPtr<CefBrowser> pBrowser) override;

private:
	CBestClientBrowserImpl *m_pOwner;

	IMPLEMENT_REFCOUNTING(CBestClientBrowserHandler);
};

class CBestClientBrowserImpl
{
public:
	CBestClientBrowserImpl(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage) :
		m_pClient(pClient),
		m_pGraphics(pGraphics),
		m_pStorage(pStorage)
	{
		str_copy(m_aStatus, "CEF not initialized", sizeof(m_aStatus));
		str_copy(m_aCurrentUrl, "https://www.google.com/", sizeof(m_aCurrentUrl));
	}

	void OnRender()
	{
		if(m_Initialized)
			CefDoMessageLoopWork();

		if(m_Visible && IsValidWindowHandle(m_hBrowser))
			ApplyBounds();
	}

	void OnWindowResize()
	{
		if(m_Visible && IsValidWindowHandle(m_hBrowser))
			ApplyBounds();
	}

	void Shutdown()
	{
		Hide();
		if(m_pBrowser)
		{
			m_pBrowser->GetHost()->CloseBrowser(true);
			m_pBrowser = nullptr;
			ClearWindowHandle(m_hBrowser);
		}

		m_pHandler = nullptr;

		if(m_Initialized)
		{
			CefDoMessageLoopWork();
			CefShutdown();
			m_Initialized = false;
		}

#if defined(CONF_PLATFORM_LINUX)
		if(m_pX11Display != nullptr)
		{
			XCloseDisplay(m_pX11Display);
			m_pX11Display = nullptr;
		}
#endif
	}

	void Show(int X, int Y, int Width, int Height, const char *pUrl)
	{
		if(!EnsureInitialized())
			return;

		m_X = maximum(X, 0);
		m_Y = maximum(Y, 0);
		m_Width = maximum(Width, 1);
		m_Height = maximum(Height, 1);
		m_Visible = true;

		if(pUrl != nullptr && pUrl[0] != '\0' && str_comp(m_aCurrentUrl, pUrl) != 0)
		{
			str_copy(m_aCurrentUrl, pUrl, sizeof(m_aCurrentUrl));
			if(m_pBrowser)
				m_pBrowser->GetMainFrame()->LoadURL(m_aCurrentUrl);
		}

		EnsureBrowser();
		ApplyBounds();
	}

	void Hide()
	{
		m_Visible = false;
		if(IsValidWindowHandle(m_hBrowser))
		{
#if defined(CONF_FAMILY_WINDOWS)
			ShowWindow(m_hBrowser, SW_HIDE);
#else
			if(m_pX11Display != nullptr)
			{
				XUnmapWindow(m_pX11Display, m_hBrowser);
				XFlush(m_pX11Display);
			}
#endif
		}
	}

	bool IsAvailable() const
	{
		return m_Initialized && IsValidWindowHandle(m_hBrowser);
	}

	const char *Status() const
	{
		return m_aStatus;
	}

	void OnAfterCreated(CefRefPtr<CefBrowser> pBrowser)
	{
		m_pBrowser = pBrowser;
		m_hBrowser = pBrowser->GetHost()->GetWindowHandle();
		ApplyBounds();
		str_copy(m_aStatus, "CEF browser is ready", sizeof(m_aStatus));
		log_info("bestclient-cef", "browser created");
	}

	void OnBeforeClose(CefRefPtr<CefBrowser> pBrowser)
	{
		if(m_pBrowser && pBrowser && m_pBrowser->IsSame(pBrowser))
		{
			m_pBrowser = nullptr;
			ClearWindowHandle(m_hBrowser);
			str_copy(m_aStatus, "CEF browser closed", sizeof(m_aStatus));
		}
	}

private:
	bool EnsureInitialized()
	{
		if(m_Initialized)
			return true;
		if(m_InitializationAttempted)
			return false;

		m_InitializationAttempted = true;
		m_hParent = NativeWindowHandleFromPointer(m_pGraphics->NativeWindowHandle());
		if(!IsValidWindowHandle(m_hParent))
		{
			str_copy(m_aStatus, "CEF init failed: missing native window handle", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}

#if defined(CONF_PLATFORM_LINUX)
		m_pX11Display = XOpenDisplay(nullptr);
		if(m_pX11Display == nullptr)
		{
			str_copy(m_aStatus, "CEF init failed: unable to open X11 display", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}
#endif

		m_pStorage->CreateFolder("BestClient", IStorage::TYPE_SAVE);
		m_pStorage->CreateFolder("BestClient/cef_cache", IStorage::TYPE_SAVE);

		char aCachePath[IO_MAX_PATH_LENGTH];
		char aLogPath[IO_MAX_PATH_LENGTH];
		m_pStorage->GetCompletePath(IStorage::TYPE_SAVE, "BestClient/cef_cache", aCachePath, sizeof(aCachePath));
		m_pStorage->GetCompletePath(IStorage::TYPE_SAVE, "BestClient/cef.log", aLogPath, sizeof(aLogPath));

		const std::filesystem::path ModulePath = GetModulePath();
		const std::filesystem::path ModuleDir = GetModuleDirectory();
		if(ModulePath.empty() || ModuleDir.empty())
		{
			str_copy(m_aStatus, "CEF init failed: unable to resolve module path", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}

		CefMainArgs MainArgs = MakeCefMainArgs();
		CefSettings Settings;
		Settings.no_sandbox = true;
		Settings.multi_threaded_message_loop = false;
		Settings.external_message_pump = false;
		Settings.command_line_args_disabled = false;

		SetCefPath(Settings.browser_subprocess_path, ModulePath);
		SetCefPath(Settings.resources_dir_path, ModuleDir);
		SetCefPath(Settings.locales_dir_path, ModuleDir / "locales");
		SetCefUtf8Path(Settings.cache_path, aCachePath);
		SetCefUtf8Path(Settings.log_file, aLogPath);

		CefRefPtr<CBestClientCefApp> pApp(new CBestClientCefApp);
		if(!CefInitialize(MainArgs, Settings, pApp.get(), nullptr))
		{
			str_copy(m_aStatus, "CEF init failed: CefInitialize returned false", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}

		m_Initialized = true;
		str_copy(m_aStatus, "CEF initialized", sizeof(m_aStatus));
		log_info("bestclient-cef", "initialized");
		return true;
	}

	void EnsureBrowser()
	{
		if(m_pBrowser || !IsValidWindowHandle(m_hParent))
			return;

		CefWindowInfo WindowInfo;
		WindowInfo.SetAsChild(m_hParent, CefRect(m_X, m_Y, m_Width, m_Height));

		CefBrowserSettings BrowserSettings;
		m_pHandler = new CBestClientBrowserHandler(this);
		m_pBrowser = CefBrowserHost::CreateBrowserSync(WindowInfo, m_pHandler, m_aCurrentUrl, BrowserSettings, nullptr, nullptr);
		if(!m_pBrowser)
		{
			str_copy(m_aStatus, "CEF browser creation failed", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
		}
	}

	void ApplyBounds()
	{
		if(!IsValidWindowHandle(m_hBrowser))
			return;

#if defined(CONF_FAMILY_WINDOWS)
		SetWindowPos(m_hBrowser, HWND_TOP, m_X, m_Y, m_Width, m_Height, SWP_NOACTIVATE);
		ShowWindow(m_hBrowser, m_Visible ? SW_SHOW : SW_HIDE);
#else
		if(m_pX11Display != nullptr)
		{
			XMoveResizeWindow(m_pX11Display, m_hBrowser, m_X, m_Y, static_cast<unsigned int>(m_Width), static_cast<unsigned int>(m_Height));
			if(m_Visible)
				XMapRaised(m_pX11Display, m_hBrowser);
			else
				XUnmapWindow(m_pX11Display, m_hBrowser);
			XFlush(m_pX11Display);
		}
#endif
		if(m_pBrowser)
		{
			m_pBrowser->GetHost()->NotifyMoveOrResizeStarted();
			m_pBrowser->GetHost()->WasResized();
		}
	}

	IClient *m_pClient;
	IGraphics *m_pGraphics;
	IStorage *m_pStorage;
	bool m_InitializationAttempted = false;
	bool m_Initialized = false;
	bool m_Visible = false;
	int m_X = 0;
	int m_Y = 0;
	int m_Width = 1;
	int m_Height = 1;
	CefWindowHandle m_hParent = {};
	CefWindowHandle m_hBrowser = {};
#if defined(CONF_PLATFORM_LINUX)
	Display *m_pX11Display = nullptr;
#endif
	CefRefPtr<CefBrowser> m_pBrowser;
	CefRefPtr<CBestClientBrowserHandler> m_pHandler;
	char m_aCurrentUrl[256];
	char m_aStatus[256];
};

void CBestClientBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> pBrowser)
{
	m_pOwner->OnAfterCreated(pBrowser);
}

void CBestClientBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> pBrowser)
{
	m_pOwner->OnBeforeClose(pBrowser);
}

CBestClientBrowser::CBestClientBrowser(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage) :
	m_pImpl(new CBestClientBrowserImpl(pClient, pGraphics, pStorage))
{
}

CBestClientBrowser::~CBestClientBrowser()
{
	delete m_pImpl;
}

void CBestClientBrowser::OnRender()
{
	m_pImpl->OnRender();
}

void CBestClientBrowser::OnWindowResize()
{
	m_pImpl->OnWindowResize();
}

void CBestClientBrowser::Shutdown()
{
	m_pImpl->Shutdown();
}

void CBestClientBrowser::Show(int X, int Y, int Width, int Height, const char *pUrl)
{
	m_pImpl->Show(X, Y, Width, Height, pUrl);
}

void CBestClientBrowser::Hide()
{
	m_pImpl->Hide();
}

bool CBestClientBrowser::IsAvailable() const
{
	return m_pImpl->IsAvailable();
}

const char *CBestClientBrowser::Status() const
{
	return m_pImpl->Status();
}

int BestClientCefExecuteSubprocess(int argc, const char **argv)
{
#if defined(CONF_FAMILY_WINDOWS)
	(void)argc;
	(void)argv;
	CefMainArgs MainArgs = MakeCefMainArgs();
#else
	g_CefArgc = argc;
	g_pCefArgv = argv;
	CefMainArgs MainArgs = MakeCefMainArgs();
#endif
	CefRefPtr<CBestClientCefApp> pApp(new CBestClientCefApp);
	return CefExecuteProcess(MainArgs, pApp.get(), nullptr);
}

#else

class CBestClientBrowserImpl
{
public:
	CBestClientBrowserImpl(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage)
	{
		(void)pClient;
		(void)pGraphics;
		(void)pStorage;
	}

	void OnRender() {}
	void OnWindowResize() {}
	void Shutdown() {}
	void Show(int X, int Y, int Width, int Height, const char *pUrl)
	{
		(void)X;
		(void)Y;
		(void)Width;
		(void)Height;
		(void)pUrl;
	}
	void Hide() {}
	bool IsAvailable() const { return false; }
	const char *Status() const { return "CEF is only available in Windows/Linux x86_64 builds with CONF_BESTCLIENT_CEF enabled"; }
};

CBestClientBrowser::CBestClientBrowser(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage) :
	m_pImpl(new CBestClientBrowserImpl(pClient, pGraphics, pStorage))
{
}

CBestClientBrowser::~CBestClientBrowser()
{
	delete m_pImpl;
}

void CBestClientBrowser::OnRender()
{
	m_pImpl->OnRender();
}

void CBestClientBrowser::OnWindowResize()
{
	m_pImpl->OnWindowResize();
}

void CBestClientBrowser::Shutdown()
{
	m_pImpl->Shutdown();
}

void CBestClientBrowser::Show(int X, int Y, int Width, int Height, const char *pUrl)
{
	m_pImpl->Show(X, Y, Width, Height, pUrl);
}

void CBestClientBrowser::Hide()
{
	m_pImpl->Hide();
}

bool CBestClientBrowser::IsAvailable() const
{
	return m_pImpl->IsAvailable();
}

const char *CBestClientBrowser::Status() const
{
	return m_pImpl->Status();
}

int BestClientCefExecuteSubprocess(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	return -1;
}

#endif
