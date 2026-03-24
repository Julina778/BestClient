// This file can be included several times.

#ifndef CONFIG_DOMAIN
#error "CONFIG_DOMAIN macro not defined"
#define CONFIG_DOMAIN(Name, ConfigPath, HasVars) ;
#endif

CONFIG_DOMAIN(DDNET, "settings_ddnet.cfg", true)
CONFIG_DOMAIN(BestClient, "settings_BestClient.cfg", true)
CONFIG_DOMAIN(BestClientPROFILES, "BestClient_profiles.cfg", false)
CONFIG_DOMAIN(BestClientCHATBINDS, "BestClient_chatbinds.cfg", false)
CONFIG_DOMAIN(BestClientWARLIST, "BestClient_warlist.cfg", false)
CONFIG_DOMAIN(BestClientSAVES, "BestClient_saves.cfg", false)
