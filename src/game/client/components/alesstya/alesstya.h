#ifndef GAME_CLIENT_COMPONENTS_ALESSTYA_ALESSTYA_H
#define GAME_CLIENT_COMPONENTS_ALESSTYA_ALESSTYA_H

#include <engine/console.h>
#include <game/client/component.h>

#define MAX_COMPONENT_LEN 16
#define MAX_COMPONENTS_ENABLED 8

class CAlesstya : public CComponent
{
	struct CUiComponent
	{
		char m_aName[MAX_COMPONENT_LEN];
		char m_aNoteShort[16];
		char m_aNoteLong[2048];
	};
	CUiComponent m_aEnabledComponents[MAX_COMPONENTS_ENABLED];
	void SaveSkins();
	void RestoreSkins();
	static void ConchainSkinStealer(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	virtual void OnShutdown() override;
	virtual void OnRender() override;

public:
	CAlesstya();
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;

	virtual void OnConsoleInit() override;

	void EnableComponent(const char *pComponent, const char *pNoteShort = 0, const char *pNoteLong = 0);
	void DisableComponent(const char *pComponent);
	bool SetComponentNoteShort(const char *pComponent, const char *pNoteShort = 0);
	bool SetComponentNoteLong(const char *pComponent, const char *pNoteLong = 0);
	void UpdateComponents();
};

#endif
