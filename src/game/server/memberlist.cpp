/* (c) XXLTomate (xxltomate@v93.eu) -- MemberList */

#include "gamecontext.h"
#include "memberlist.h"
#include <base/tl/sorted_array.h>
#include <engine/shared/config.h>
#include <engine/external/md5/md5.h>
#include <engine/server/server.h>
#include <string.h>
#include <sstream>
#include <fstream>


static LOCK gs_MemberLock = 0;

std::string SaveMemberFile()
{
	std::ostringstream oss;
	oss << "MemberList.dtb";
	return oss.str();
}

CMemberList::CPlayerMember::CPlayerMember(const char *pName, const char *pPass, int AuthLvl)
{
	str_copy(m_aName, pName, sizeof(m_aName));
	str_copy(m_aPass, pPass, sizeof(m_aPass));
	m_AuthLvl = clamp(AuthLvl, -1, (int)CServer::AUTHED_ADMIN);
}

CMemberList::CMemberList(CGameContext *pGameServer) : m_pGameServer(pGameServer), m_pServer((CServer*)pGameServer->Server())
{
	if(gs_MemberLock == 0)
		gs_MemberLock = lock_create();

	Init();
}

CMemberList::~CMemberList()
{
	lock_wait(gs_MemberLock);

	// clear list
	m_List.clear();

	lock_release(gs_MemberLock);
}

void CMemberList::Init()
{
	lock_wait(gs_MemberLock);
	std::fstream f;
	f.open(SaveMemberFile().c_str(), std::ios::in);

	while(!f.eof() && !f.fail())
	{
		std::string TmpName, TmpPass, TmpAuthLvl;
		std::getline(f, TmpName);
		if(!f.eof() && TmpName != "")
		{
			std::getline(f, TmpPass);
			std::getline(f, TmpAuthLvl);
			m_List.add(*new CPlayerMember(TmpName.c_str(), TmpPass.c_str(), atoi(TmpAuthLvl.c_str())));
		}
	}
	f.close();
	lock_release(gs_MemberLock);
}

void CMemberList::SaveListThread(void *pUser)
{
	CMemberList *pSelf = (CMemberList *)pUser;
	lock_wait(gs_MemberLock);
	std::fstream f;
	f.open(SaveMemberFile().c_str(), std::ios::out);

	if(!f.fail())
	{
		int t = 0;
		for(sorted_array<CPlayerMember>::range r = pSelf->m_List.all(); !r.empty(); r.pop_front())
		{
			f << r.front().m_aName << std::endl << r.front().m_aPass << std::endl << r.front().m_AuthLvl << std::endl;
			t++;
			if(t%50 == 0)
				thread_sleep(1);
		}
	}
	f.close();
	lock_release(gs_MemberLock);
}

void CMemberList::Save()
{
	void *pSaveThread = thread_create(SaveListThread, this);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)pSaveThread);
#endif
}

CMemberList::CPlayerMember *CMemberList::SearchName(const char *pName, int *pPosition, bool NoCase)
{
	CPlayerMember *pPlayer = 0;
	int Pos = 1;
	int Found = 0;

	for(sorted_array<CPlayerMember>::range r = m_List.all(); !r.empty(); r.pop_front())
	{
		if(str_find_nocase(r.front().m_aName, pName))
		{
			if(pPosition)
				*pPosition = Pos;
			if(NoCase)
			{
				Found++;
				pPlayer = &r.front();
			}
			if(!strcmp(r.front().m_aName, pName))
				return &r.front();
		}
		Pos++;
	}
	if(Found > 1)
	{
		if(pPosition)
			*pPosition = -1;
		return 0;
	}
	return pPlayer;
}

void CMemberList::UpdatePlayer(int ClientID, const char* pPass, int AuthLvl)
{
	const char *pName = Server()->ClientName(ClientID);
	lock_wait(gs_MemberLock);
	CPlayerMember *pPlayer = SearchList(ClientID, 0);

	if(pPlayer)
	{
		pPlayer->m_AuthLvl = clamp(AuthLvl, -1, (int)CServer::AUTHED_ADMIN);
		str_copy(pPlayer->m_aName, pName, sizeof(pPlayer->m_aName));
		str_copy(pPlayer->m_aPass, pPass, sizeof(pPlayer->m_aPass));

		sort(m_List.all());
	}
	else
		m_List.add(*new CPlayerMember(pName, pPass, AuthLvl));

	lock_release(gs_MemberLock);
	Save();
}

void CMemberList::LoadMember(int ClientID, CGameContext *pSelf)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);

	if(pPlayer)
	{
		if (pSelf->m_apPlayers[ClientID]->m_Authed > pPlayer->m_AuthLvl && pSelf->m_apPlayers[ClientID]->m_Authed != 0)
			pPlayer->m_AuthLvl = pSelf->m_apPlayers[ClientID]->m_Authed;
		lock_wait(gs_MemberLock);
		lock_release(gs_MemberLock);
		Save();

		// set Level
		if (pPlayer->m_AuthLvl > m_pServer->AUTHED_NO)
		{
			char buf[128]="Authentication successful. Remote console access granted for ClientID=%d with level=%d";
			pSelf->Server()->SetRconLevel(ClientID,pPlayer->m_AuthLvl);
			str_format(buf,sizeof(buf),buf,ClientID,pPlayer->m_AuthLvl);
			pSelf->Server()->SendRconLine(ClientID, buf);
			dbg_msg("server", "'%s' ClientID=%d authed with Level=%d", pSelf->Server()->ClientName(ClientID), ClientID, pPlayer->m_AuthLvl);
		}
	}
}

void CMemberList::SaveList(int ClientID, const char* pPass, CGameContext *pSelf, bool ForceAuth)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);
	int SetAuthLvl = -1;

	if(ForceAuth){
		UpdatePlayer(ClientID, pPass, SetAuthLvl);
		return;
	}

	if(!pPlayer)
	{
		if (pSelf->m_apPlayers[ClientID]->m_Authed > 0)
			SetAuthLvl = pSelf->m_apPlayers[ClientID]->m_Authed;
		else
			SetAuthLvl = -1;
	}
	else
	{
		if (pSelf->m_apPlayers[ClientID]->m_Authed > pPlayer->m_AuthLvl)
			SetAuthLvl = pSelf->m_apPlayers[ClientID]->m_Authed;
		else
			SetAuthLvl = pPlayer->m_AuthLvl;
	}
	UpdatePlayer(ClientID, pPass, SetAuthLvl);
}

void CMemberList::Register(int ClientID, const char* pPass, CGameContext *pSelf)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);
	char aBuf[256];

	if (!pPlayer)
	{
		SaveList(ClientID, md5(pPass).c_str(), pSelf, false);
		str_format(aBuf, sizeof(aBuf), "Registration successful.");
	}
	else
		str_format(aBuf, sizeof(aBuf), "%s is already a registered name.", pSelf->Server()->ClientName(ClientID));

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "member", aBuf);
}

void CMemberList::Login(int ClientID, const char* pPass, CGameContext *pSelf)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);
	char aBuf[256];

	if (!pPlayer)
		str_format(aBuf, sizeof(aBuf), "You are not registered, type /register <pass> first.");
	else if(pPlayer->m_AuthLvl < pSelf->m_apPlayers[ClientID]->m_Authed && pSelf->m_apPlayers[ClientID]->m_Authed > m_pServer->AUTHED_NO)
	{
		pPlayer->m_AuthLvl = pSelf->m_apPlayers[ClientID]->m_Authed;
		pSelf->m_apPlayers[ClientID]->m_IsMember = true;
		SaveList(ClientID, pPlayer->m_aPass , pSelf, false);
		str_format(aBuf, sizeof(aBuf), "Level updated.");
	}
	else if(pSelf->m_apPlayers[ClientID]->m_IsLoggedIn)
		str_format(aBuf, sizeof(aBuf), "You are already logged in.");
	else
	{
		if (str_comp(pPlayer->m_aPass, md5(pPass).c_str()) == 0)
		{
			LoadMember(ClientID,pSelf);
			pSelf->m_apPlayers[ClientID]->m_IsLoggedIn = true;
			str_format(aBuf, sizeof(aBuf), "Login successful.");
			if (pPlayer->m_AuthLvl >= 0)
				pSelf->m_apPlayers[ClientID]->m_IsMember = true;
		}
		else
			str_format(aBuf, sizeof(aBuf), "Wrong password.");
	}
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "member", aBuf);
}

void CMemberList::Member(int ClientID, CGameContext *pSelf)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);
	char aBuf[256];

	if (!pPlayer)
		str_format(aBuf, sizeof(aBuf), "%s is not a registered name.", pSelf->Server()->ClientName(ClientID));
	else
	{
		if (pPlayer->m_AuthLvl > 0)
			str_format(aBuf, sizeof(aBuf), "%s has a higher rank, he is already member.", pSelf->Server()->ClientName(ClientID));
		else if (pPlayer->m_AuthLvl == 0)
			str_format(aBuf, sizeof(aBuf), "%s is already member.", pSelf->Server()->ClientName(ClientID));
		else
		{
			str_format(aBuf, sizeof(aBuf), "%s is NOW member.", pSelf->Server()->ClientName(ClientID));
			pPlayer->m_AuthLvl = 0;//Set member
			pSelf->m_apPlayers[ClientID]->m_IsMember = true; //Set member
			SaveList(ClientID, pPlayer->m_aPass , pSelf, false); //Save to file
		}
	}
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "member", aBuf);
}

void CMemberList::UnMember(int ClientID, CGameContext *pSelf)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);
	char aBuf[256];

	if (!pPlayer)
		str_format(aBuf, sizeof(aBuf), "%s is not registered.", pSelf->Server()->ClientName(ClientID));
	else
	{
		if (pPlayer->m_AuthLvl > 0)
			str_format(aBuf, sizeof(aBuf), "You can't unmember a helper/moder/admin/sadmin.");
		else if (pPlayer->m_AuthLvl == -1)
			str_format(aBuf, sizeof(aBuf), "%s is not a member.", pSelf->Server()->ClientName(ClientID));
		else
		{
			str_format(aBuf, sizeof(aBuf), "%s is NOW unmembered.", pSelf->Server()->ClientName(ClientID));
			pPlayer->m_AuthLvl = m_pServer->AUTHED_NO;
			pSelf->m_apPlayers[ClientID]->m_IsMember = false;
			SaveList(ClientID, pPlayer->m_aPass , pSelf, true);
		}
	}
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "member", aBuf);
}

void CMemberList::Check(int ClientID, CGameContext *pSelf)
{
	CPlayerMember *pPlayer = SearchList(ClientID, 0);
	char aBuf[256];

	if (!pPlayer)
		str_format(aBuf, sizeof(aBuf), "%s is not registered.", pSelf->Server()->ClientName(ClientID));
	else
		str_format(aBuf, sizeof(aBuf), "%s: logged in=%d member=%d", pSelf->Server()->ClientName(ClientID), pSelf->m_apPlayers[ClientID]->m_IsLoggedIn, pSelf->m_apPlayers[ClientID]->m_IsMember);

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "member", aBuf);
}
