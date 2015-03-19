/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/version.h>
#include <game/generated/nethash.cpp>
#if defined(CONF_SQL)
#include <game/server/score/sql_score.h>
#endif

bool CheckClientID(int ClientID);
bool CheckRights(int ClientID, int Victim, CGameContext *GameContext);

void CGameContext::ConGoLeft(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, -1, 0);
}

void CGameContext::ConGoRight(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 1, 0);
}

void CGameContext::ConGoDown(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 0, 1);
}

void CGameContext::ConGoUp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 0, -1);
}

void CGameContext::ConMove(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, pResult->GetInteger(0),
			pResult->GetInteger(1));
}

void CGameContext::ConMoveRaw(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, pResult->GetInteger(0),
			pResult->GetInteger(1), true);
}

void CGameContext::MoveCharacter(int ClientID, int X, int Y, bool Raw)
{
	CCharacter* pChr = GetPlayerChar(ClientID);

	if (!pChr)
		return;

	pChr->Core()->m_Pos.x += ((Raw) ? 1 : 32) * X;
	pChr->Core()->m_Pos.y += ((Raw) ? 1 : 32) * Y;
	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConSetlvl3(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && Victim != pResult->m_ClientID)
		pServ->SetRconLevel(Victim, pServ->AUTHED_SUBADMIN);
}

void CGameContext::ConSetlvl2(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && Victim != pResult->m_ClientID)
		pServ->SetRconLevel(Victim, pServ->AUTHED_MOD);
}

void CGameContext::ConSetlvl1(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && Victim != pResult->m_ClientID)
		pServ->SetRconLevel(Victim, pServ->AUTHED_HELPER);
}

void CGameContext::ConKillPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int Victim = pResult->GetVictim();

	if (pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->KillCharacter(WEAPON_GAME);
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%s was killed by %s",
				pSelf->Server()->ClientName(Victim),
				pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}
}

void CGameContext::ConNinja(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_NINJA, false);
}

void CGameContext::ConSuper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int Victim = pResult->GetVictim();
	if (!CheckRights(pResult->m_ClientID, Victim, (CGameContext *)pUserData))
		return;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr && !pChr->m_Super)
	{
		pChr->m_Super = true;
		pChr->UnFreeze();
		pChr->m_TeamBeforeSuper = pChr->Team();
		pChr->Teams()->SetCharacterTeam(Victim, TEAM_SUPER);
		pChr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConUnSuper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int Victim = pResult->GetVictim();
	if (!CheckRights(pResult->m_ClientID, Victim, (CGameContext *)pUserData))
		return;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr && pChr->m_Super)
	{
		pChr->m_Super = false;
		pChr->Teams()->SetForceCharacterTeam(Victim,
				pChr->m_TeamBeforeSuper);
	}
}

void CGameContext::ConShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_SHOTGUN, false);
}

void CGameContext::ConGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GRENADE, false);
}

void CGameContext::ConRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_RIFLE, false);
}

void CGameContext::ConWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -1, false);
}

void CGameContext::ConUnShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_SHOTGUN, true);
}

void CGameContext::ConUnGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GRENADE, true);
}

void CGameContext::ConUnRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_RIFLE, true);
}

void CGameContext::ConUnWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -1, true);
}

void CGameContext::ConAddWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, pResult->GetInteger(0), false);
}

void CGameContext::ConRemoveWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, pResult->GetInteger(0), true);
}

void CGameContext::ModifyWeapons(IConsole::IResult *pResult, void *pUserData,
		int Weapon, bool Remove)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckRights(pResult->m_ClientID, pResult->GetVictim(), pSelf))
		return;
	int ClientID = pResult->GetVictim();
	if (clamp(Weapon, -1, NUM_WEAPONS - 1) != Weapon)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
				"invalid weapon id");
		return;
	}

	CCharacter* pChr = GetPlayerChar(ClientID);
	if (!pChr)
		return;

	if (Weapon == -1)
	{
		if (Remove
				&& (pChr->GetActiveWeapon() == WEAPON_SHOTGUN
						|| pChr->GetActiveWeapon() == WEAPON_GRENADE
						|| pChr->GetActiveWeapon() == WEAPON_RIFLE))
			pChr->SetActiveWeapon(WEAPON_GUN);

		if (Remove)
		{
			pChr->SetWeaponGot(WEAPON_SHOTGUN, false);
			pChr->SetWeaponGot(WEAPON_GRENADE, false);
			pChr->SetWeaponGot(WEAPON_RIFLE, false);
		}
		else
			pChr->GiveAllWeapons();
	}
	else if (Weapon != WEAPON_NINJA)
	{
		if (Remove && pChr->GetActiveWeapon() == Weapon)
			pChr->SetActiveWeapon(WEAPON_GUN);

		if (Remove)
			pChr->SetWeaponGot(Weapon, false);
		else
			pChr->GiveWeapon(Weapon, -1);
	}
	else
	{
		if (Remove)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
					"you can't remove ninja");
			return;
		}

		pChr->GiveNinja();
	}

	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConTeleport(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int TeleTo = pResult->GetInteger(0);
	int Tele = pResult->GetVictim();

	if (!CheckRights(pResult->m_ClientID, Tele, (CGameContext *)pUserData))
		return;

	if (pSelf->m_apPlayers[TeleTo])
	{
		CCharacter* pChr = pSelf->GetPlayerChar(Tele);
		if (pChr && pSelf->GetPlayerChar(TeleTo))
		{
			pChr->Core()->m_Pos = pSelf->m_apPlayers[TeleTo]->m_ViewPos;
			pChr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConKill(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];

	if (!pPlayer
			|| (pPlayer->m_LastKill
					&& pPlayer->m_LastKill
					+ pSelf->Server()->TickSpeed()
					* g_Config.m_SvKillDelay
					> pSelf->Server()->Tick()))
		return;

	pPlayer->m_LastKill = pSelf->Server()->Tick();
	pPlayer->KillCharacter(WEAPON_SELF);
	//pPlayer->m_RespawnTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * g_Config.m_SvSuicidePenalty;
}

void CGameContext::ConForcePause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CServer* pServ = (CServer*)pSelf->Server();
	int Victim = pResult->GetVictim();
	if (!CheckRights(pResult->m_ClientID, Victim, (CGameContext *)pUserData)) return;
	int Seconds = 0;
	if (pResult->NumArguments() > 0)
		Seconds = clamp(pResult->GetInteger(0), 0, 360);

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if (!pPlayer)
		return;

	pPlayer->m_ForcePauseTime = Seconds*pServ->TickSpeed();
	pPlayer->m_Paused = CPlayer::PAUSED_FORCE;
}

void CGameContext::Mute(IConsole::IResult *pResult, NETADDR *Addr, int Secs,
		const char *pDisplayName)
{
	char aBuf[128];
	int Found = 0;
	// find a matching mute for this ip, update expiration time if found
	for (int i = 0; i < m_NumMutes; i++)
	{
		if (net_addr_comp(&m_aMutes[i].m_Addr, Addr) == 0)
		{
			m_aMutes[i].m_Expire = Server()->Tick()
							+ Secs * Server()->TickSpeed();
			Found = 1;
		}
	}

	if (!Found) // nothing found so far, find a free slot..
	{
		if (m_NumMutes < MAX_MUTES)
		{
			m_aMutes[m_NumMutes].m_Addr = *Addr;
			m_aMutes[m_NumMutes].m_Expire = Server()->Tick()
							+ Secs * Server()->TickSpeed();
			m_NumMutes++;
			Found = 1;
		}
	}
	if (Found)
	{
		str_format(aBuf, sizeof aBuf, "'%s' has been muted for %d seconds.",
				pDisplayName, Secs);
		SendChat(-1, CHAT_ALL, aBuf);
	}
	else // no free slot found
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", "mute array is full");
}

void CGameContext::ConMute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"mutes",
			"Use either 'muteid <client_id> <seconds>' or 'muteip <ip> <seconds>'");
}

// mute through client id
void CGameContext::ConMuteID(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int Victim = pResult->GetVictim();
	if (!CheckRights(pResult->m_ClientID, Victim, (CGameContext *)pUserData)) return;

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	pSelf->Mute(pResult, &Addr, clamp(pResult->GetInteger(0), 1, 86400),
			pSelf->Server()->ClientName(Victim));
}

// mute through ip, arguments reversed to workaround parsing
void CGameContext::ConMuteIP(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	NETADDR Addr;
	if (net_addr_from_str(&Addr, pResult->GetString(0)))
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
				"Invalid network address to mute");
	}
	pSelf->Mute(pResult, &Addr, clamp(pResult->GetInteger(1), 1, 86400),
			pResult->GetString(0));
}

// unmute by mute list index
void CGameContext::ConUnmute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	char aIpBuf[64];
	char aBuf[64];
	int Victim = pResult->GetVictim();
	if (!CheckRights(pResult->m_ClientID, Victim, (CGameContext *)pUserData)) return;

	if (Victim < 0 || Victim >= pSelf->m_NumMutes)
		return;

	pSelf->m_NumMutes--;
	pSelf->m_aMutes[Victim] = pSelf->m_aMutes[pSelf->m_NumMutes];

	net_addr_str(&pSelf->m_aMutes[Victim].m_Addr, aIpBuf, sizeof(aIpBuf), false);
	str_format(aBuf, sizeof(aBuf), "Unmuted %s", aIpBuf);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);
}

// list mutes
void CGameContext::ConMutes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	char aIpBuf[64];
	char aBuf[128];
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
			"Active mutes:");
	for (int i = 0; i < pSelf->m_NumMutes; i++)
	{
		net_addr_str(&pSelf->m_aMutes[i].m_Addr, aIpBuf, sizeof(aIpBuf), false);
		str_format(
				aBuf,
				sizeof aBuf,
				"%d: \"%s\", %d seconds left",
				i,
				aIpBuf,
				(pSelf->m_aMutes[i].m_Expire - pSelf->Server()->Tick())
				/ pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);
	}
}

//Restored
void CGameContext::ConHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	if(!CheckRights(pResult->m_ClientID, Victim, (CGameContext *)pUserData)) return;

	char aBuf[128];
	int Type = pResult->GetInteger(0);

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if(!pChr)
		return;

	CServer* pServ = (CServer*)pSelf->Server();
	if(Type > 10 || Type < 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Select hammer between 0 and 10");
	}
	else
	{
		pChr->m_HammerType = Type;
		pChr->m_DDRaceState = DDRACE_CHEAT;
		str_format(aBuf, sizeof(aBuf), "Hammer of '%s' ClientID=%d setted to %d", pServ->ClientName(Victim), Victim, Type);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
	}
}

void CGameContext::ConToggleFly(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckClientID(pResult->m_ClientID)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if(!pChr)
		return;
	if(pChr->m_Super)
	{
		pChr->m_Fly = !pChr->m_Fly;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pChr->m_Fly) ? "Fly enabled" : "Fly disabled");
	}
}

void CGameContext::ConFreeze(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Seconds = -1;
	int Victim = pResult->GetVictim();

	char aBuf[128];

	if(pResult->NumArguments() == 1)
		Seconds = clamp(pResult->GetInteger(0), -2, 9999);

	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if(!pChr)
		return;

	if(pSelf->m_apPlayers[Victim])
	{
		pChr->Freeze(Seconds);
		pChr->GetPlayer()->m_RconFreeze = Seconds != -2;
		CServer* pServ = (CServer*)pSelf->Server();
		if(Seconds >= 0)
			str_format(aBuf, sizeof(aBuf), "'%s' ClientID=%d has been Frozen for %d.", pServ->ClientName(Victim), Victim, Seconds);
		else if(Seconds == -2)
		{
			pChr->m_DeepFreeze = true;
			str_format(aBuf, sizeof(aBuf), "'%s' ClientID=%d has been Deep Frozen.", pServ->ClientName(Victim), Victim);
		}
		else
			str_format(aBuf, sizeof(aBuf), "'%s' ClientID=%d is Frozen until you unfreeze him.", pServ->ClientName(Victim), Victim);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
	}

}

void CGameContext::ConUnFreeze(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	static bool Warning = false;
	char aBuf[128];
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if(!pChr)
		return;
	if(pChr->m_DeepFreeze && !Warning)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "warning", "This client is deeply frozen, repeat the command to defrost him.");
		Warning = true;
		return;
	}
	if(pChr->m_DeepFreeze && Warning)
	{
		pChr->m_DeepFreeze = false;
		Warning = false;
	}
	pChr->m_FreezeTime = 2;
	pChr->GetPlayer()->m_RconFreeze = false;
	CServer* pServ = (CServer*)pSelf->Server();
	str_format(aBuf, sizeof(aBuf), "'%s' ClientID=%d has been defrosted.", pServ->ClientName(Victim), Victim);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
}

void CGameContext::ConInvis(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[128];
	int Victim = pResult->GetVictim();

	if(!pSelf->m_apPlayers[pResult->m_ClientID])
		return;

	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Invisible = true;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(aBuf, sizeof(aBuf), "'%s' ClientID=%d is now invisible.", pServ->ClientName(Victim), Victim);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
	}
}

void CGameContext::ConVis(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(!pSelf->m_apPlayers[pResult->m_ClientID])
		return;
	char aBuf[128];
	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Invisible = false;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(aBuf, sizeof(aBuf), "'%s' ClientID=%d is visible.", pServ->ClientName(Victim), Victim);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
	}
}

//XXLmod
void CGameContext::ConSkin(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	const char *Skin = pResult->GetString(0);
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();;
	if(!pChr)
		return;

	//change skin
	str_copy(pSelf->m_apPlayers[Victim]->m_TeeInfos.m_SkinName, Skin, sizeof(pSelf->m_apPlayers[Victim]->m_TeeInfos.m_SkinName));
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s's skin changed to %s" ,pSelf->Server()->ClientName(Victim), Skin);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
}

void CGameContext::ConRename(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	const char *newName = pResult->GetString(0);
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();
	if(!pChr)
		return;

	//change name
	char oldName[MAX_NAME_LENGTH];
	str_copy(oldName, pSelf->Server()->ClientName(Victim), MAX_NAME_LENGTH);

	pSelf->Server()->SetClientName(Victim, newName);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s has changed %s's name to '%s'", pSelf->Server()->ClientName(pResult->m_ClientID), oldName, pSelf->Server()->ClientName(Victim));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);

	str_format(aBuf, sizeof(aBuf), "%s changed your name to %s.", pSelf->Server()->ClientName(pResult->m_ClientID), pSelf->Server()->ClientName(Victim));
	pSelf->SendChatTarget(Victim, aBuf);
}

void CGameContext::ConFastReload(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();
	if(!pChr)
		return;

	char aBuf[128];
	if (!pChr->m_FastReload)
	{
		pChr->m_ReloadMultiplier = 10000;
		pChr->m_FastReload = true;

		str_format(aBuf, sizeof(aBuf), "You got XXL by %s.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}
	else
	{
		pChr->m_ReloadMultiplier = 1000;
		pChr->m_FastReload = false;
		str_format(aBuf, sizeof(aBuf), "%s removed your XXL.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}

	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConRainbow(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	int Rainbowtype = clamp(pResult->GetInteger(0), 0, 2);

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();
	if(!pChr)
		return;

	char aBuf[256];
	if ((pSelf->m_apPlayers[Victim]->m_Rainbow == RAINBOW_NONE || pSelf->m_apPlayers[Victim]->m_Rainbow == RAINBOW_BLACKWHITE) && Rainbowtype <= 1)
	{
		pSelf->m_apPlayers[Victim]->m_Rainbow = RAINBOW_COLOR;

		str_format(aBuf, sizeof(aBuf), "You got rainbow by %s.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}
	else if ((pSelf->m_apPlayers[Victim]->m_Rainbow == RAINBOW_NONE || pSelf->m_apPlayers[Victim]->m_Rainbow == RAINBOW_COLOR) && Rainbowtype == 2)
	{
		pSelf->m_apPlayers[Victim]->m_Rainbow = RAINBOW_BLACKWHITE;

		str_format(aBuf, sizeof(aBuf), "You got black and white rainbow by %s.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}
	else
	{
		pSelf->m_apPlayers[Victim]->m_Rainbow = RAINBOW_NONE;
		str_format(aBuf, sizeof(aBuf), "%s removed your rainbow.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}
}

void CGameContext::ConWhisper(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CCharacter* pChr = pSelf->m_apPlayers[pResult->m_ClientID]->GetCharacter();
	const char *message = pResult->GetString(0);

	if(!pChr)
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s: %s", pSelf->Server()->ClientName(pResult->m_ClientID), message);
	pSelf->SendChatTarget(Victim, aBuf);

	str_format(aBuf, sizeof(aBuf), "-->%s: %s", pSelf->Server()->ClientName(pResult->m_ClientID), message);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
}

void CGameContext::ConScore(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	int Victim = pResult->GetVictim();
	int Score = clamp(pResult->GetInteger(0), -9999, 9999);

	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	if (!g_Config.m_SvRconScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You can't set a score, if sv_rcon_score is not enabled.");
		return;
	}

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();;
	if(!pChr)
		return;

	pSelf->m_apPlayers[Victim]->m_Score = Score;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s set score of %s to %i" ,pSelf->Server()->ClientName(pResult->m_ClientID),pSelf->Server()->ClientName(Victim), Score);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);

	str_format(aBuf, sizeof(aBuf), "%s set your to %i.", pSelf->Server()->ClientName(pResult->m_ClientID), Score);
	pSelf->SendChatTarget(Victim, aBuf);
}

void CGameContext::ConBlood(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	int Victim = pResult->GetVictim();

	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();;
	if(!pChr)
		return;

	char aBuf[128];
	if (!pChr->m_Bloody)
	{
		pChr->m_Bloody = true;

		str_format(aBuf, sizeof(aBuf), "You got bloody by %s.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}
	else
	{
		pChr->m_Bloody = false;
		str_format(aBuf, sizeof(aBuf), "%s removed your blood.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(Victim, aBuf);
	}
}

void CGameContext::ConIceHammer(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();
	if (!pChr)
		return;

	pChr->m_IceHammer = true;
}

void CGameContext::ConUnIceHammer(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter* pChr = pSelf->m_apPlayers[Victim]->GetCharacter();
	if (!pChr)
		return;

	pChr->m_IceHammer = false;
}

void CGameContext::ConSetlvl4(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim] && Victim != pResult->m_ClientID)
		pServ->SetRconLevel(Victim, pServ->AUTHED_ADMIN);
}

void CGameContext::ConTest(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckClientID(pResult->m_ClientID)) return;
	int Victim = pResult->m_ClientID;
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *message = pResult->GetString(0);
	char aBuf[256];

	str_format(aBuf, sizeof(aBuf), "ClientID:%i|First arg:%s", pResult->m_ClientID, message);

	pSelf->SendChatTarget(Victim, aBuf);

//
//	CGameContext *pSelf = (CGameContext *)pUserData;
//	CCharacter* pChr = pSelf->m_apPlayers[pResult->ClientID]->GetCharacter();
//	if(!pChr)
//		return;

//	str_format(aBuf, sizeof(aBuf), "md5:%s" , md5(message));


	//pChr->m_is_bloody = 1;
	//~ pSelf->m_apPlayers[ClientID]->m_Score = (Score()->PlayerData(ClientID)->m_BestTime)?Score()->PlayerData(ClientID)->m_BestTime:0;
	//~ pSelf->m_apPlayers[ClientID]->m_Score = 123;
	//~ char aBuf[256];
	//~ str_format(aBuf, sizeof(aBuf), "%s: %s" ,pSelf->Server()->ClientName(ClientID),message);
	//~ str_format(aBuf, sizeof(aBuf), "Teeinfo:%i" , pSelf->m_apPlayers[ClientID]->m_TeeInfos.m_ColorBody);
	//~ pResult->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
	//~ pSelf->SendChatTarget(Victim, aBuf);

}

void CGameContext::ConSetJumps(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	CCharacter *pChr = pSelf->m_apPlayers[Victim]->GetCharacter();
	if (!pChr)
		return;

	pChr->Core()->m_MaxJumps = clamp(pResult->GetInteger(0), 0, 9999);
	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConCheckMember(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	pSelf->MemberList->Check(Victim, pSelf);
}

void CGameContext::ConMember(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	pSelf->MemberList->Member(Victim, pSelf);
}

void CGameContext::ConUnMember(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckRights(pResult->m_ClientID, pResult->GetVictim(), (CGameContext *)pUserData)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	pSelf->MemberList->UnMember(Victim, pSelf);
}

bool CheckRights(int ClientID, int Victim, CGameContext *GameContext)
{
	if(!CheckClientID(ClientID)) return false;
	if(!CheckClientID(Victim)) return false;

	if (ClientID == Victim)
		return true;

	if (!GameContext->m_apPlayers[ClientID] || !GameContext->m_apPlayers[Victim])
		return false;

	if(GameContext->m_apPlayers[ClientID]->m_Authed <= GameContext->m_apPlayers[Victim]->m_Authed)
		return false;

	return true;
}

void CGameContext::ConList(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->m_ClientID;
	if(!CheckClientID(ClientID)) return;

	char zerochar = 0;
	if(pResult->NumArguments() > 0)
		pSelf->List(ClientID, pResult->GetString(0));
	else
		pSelf->List(ClientID, &zerochar);
}
