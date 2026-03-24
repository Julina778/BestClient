#ifndef GAME_CLIENT_COMPONENTS_BestClient_BG_DRAW_H
#define GAME_CLIENT_COMPONENTS_BestClient_BG_DRAW_H

#include <engine/client/enums.h>
#include <engine/console.h>

#include <game/client/component.h>

#include <array>
#include <list>
#include <memory>
#include <optional>

#define BG_DRAW_MAX_POINTS_PER_ITEM 1024

class CBgDrawItem;
class IJob;

class CBgDraw : public CComponent
{
private:
	float m_NextAutoSave;
	bool m_Dirty;
	std::shared_ptr<IJob> m_pAutoSaveJob;
	std::array<std::optional<CBgDrawItem *>, NUM_DUMMIES> m_apActiveItems;
	std::array<std::optional<vec2>, NUM_DUMMIES> m_aLastPos;
	std::list<CBgDrawItem> *m_pvItems;
	static void ConBgDraw(IConsole::IResult *pResult, void *pUserData);
	static void ConBgDrawErase(IConsole::IResult *pResult, void *pUserData);
	static void ConBgDrawReset(IConsole::IResult *pResult, void *pUserData);
	static void ConBgDrawSave(IConsole::IResult *pResult, void *pUserData);
	static void ConBgDrawLoad(IConsole::IResult *pResult, void *pUserData);
	void Reset();
	void ResetInputState();
	bool IsEnabled() const;
	bool Save(const char *pFile, bool Verbose);
	bool Load(const char *pFile, bool Verbose);
	bool HasActiveInput() const;
	bool HasRenderWork() const;
	template<typename... T>
	CBgDrawItem *AddItem(T &&...Args);
	void MakeSpaceFor(int Count);

public:
	enum class InputMode
	{
		NONE = 0,
		DRAW = 1,
		ERASE = 2,
	};
	std::array<InputMode, NUM_DUMMIES> m_aInputData;

	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnRender() override;
	void OnStateChange(int NewState, int OldState) override;
	void OnMapLoad() override;
	void OnShutdown() override;

	CBgDraw();
	~CBgDraw() override;
};

#endif
