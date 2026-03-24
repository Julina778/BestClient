#ifndef ENGINE_DISCORD_H
#define ENGINE_DISCORD_H

#include "kernel.h"

#include <base/types.h>

#include <engine/serverbrowser.h>

class IDiscord : public IInterface
{
	MACRO_INTERFACE("discord")
public:
	virtual ~IDiscord() {}
	virtual void SetGameInfo(const class CServerInfo *pServerInfo,
		const char *pMapName,
		const char *pPlayerName,
		const char *pSkinName,
		bool ShowMap,
		bool Registered) = 0;
	virtual void ClearGameInfo() = 0;
	virtual void Update(bool IsEnabled) = 0;
	virtual void UpdateServerInfo(const class CServerInfo *pServerInfo, const char *pMapName) = 0;
	virtual void UpdatePlayerCount(int PlayerCount) = 0;
};

IDiscord *CreateDiscord();

#endif // ENGINE_DISCORD_H
