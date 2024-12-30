#include <base/system.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/player.h>
#include <game/server/score.h>

void CPlayer::ResetStats()
{
	m_Kills = 0;
	m_Deaths = 0;
	m_Stats.Reset();
}

void CPlayer::WarmupAlert()
{
	// 0.7 has client side infinite warmup support
	// so we do only need the broadcast for 0.6 players
	if(Server()->IsSixup(GetCid()))
		return;

	m_SentWarmupAlerts++;
	if(m_SentWarmupAlerts < 3)
	{
		GameServer()->SendBroadcast("This is a warmup game. Call a restart vote to start.", GetCid());
	}
}

const char *CPlayer::GetTeamStr() const
{
	if(GetTeam() == TEAM_SPECTATORS)
		return "spectator";

	if(GameServer()->m_pController && !GameServer()->m_pController->IsTeamPlay())
		return "game";

	if(GetTeam() == TEAM_RED)
		return "red";
	return "blue";
}

void CPlayer::AddScore(int Score)
{
	if(GameServer()->m_pController && GameServer()->m_pController->IsWarmup())
	{
		WarmupAlert();
		return;
	}

	// never decrement the tracked score
	// so fakers can not remove points from others
	if(Score > 0 && GameServer()->m_pController && GameServer()->m_pController->IsStatTrack())
		m_Stats.m_Points += Score;

	m_Score = m_Score.value_or(0) + Score;
}

void CPlayer::AddKills(int Amount)
{
	if(GameServer()->m_pController->IsStatTrack())
		m_Stats.m_Kills += Amount;

	m_Kills += Amount;
}

void CPlayer::AddDeaths(int Amount)
{
	if(GameServer()->m_pController->IsStatTrack())
		m_Stats.m_Deaths += Amount;

	m_Deaths += Amount;
}

void CPlayer::InstagibTick()
{
	if(m_StatsQueryResult != nullptr && m_StatsQueryResult->m_Completed)
	{
		ProcessStatsResult(*m_StatsQueryResult);
		m_StatsQueryResult = nullptr;
	}
	if(m_FastcapQueryResult != nullptr && m_FastcapQueryResult->m_Completed)
	{
		ProcessStatsResult(*m_FastcapQueryResult);
		m_FastcapQueryResult = nullptr;
	}
	if(!GameServer()->m_World.m_Paused)
	{
		if(!m_pCharacter)
			m_Spawning = true;

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
			{
				ProcessPause();
				if(!m_Paused)
					m_ViewPos = m_pCharacter->m_Pos;
			}
			else if(!m_pCharacter->IsPaused())
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning && !m_WeakHookSpawn)
			TryRespawn();
	}
	else
	{
		++m_JoinTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
	}

	m_TuneZoneOld = m_TuneZone; // determine needed tunings with viewpos
	int CurrentIndex = GameServer()->Collision()->GetMapIndex(m_ViewPos);
	m_TuneZone = GameServer()->Collision()->IsTune(CurrentIndex);

	if(m_TuneZone != m_TuneZoneOld) // don't send tunings all the time
	{
		GameServer()->SendTuningParams(m_ClientId, m_TuneZone);
	}

	if(m_OverrideEmoteReset >= 0 && m_OverrideEmoteReset <= Server()->Tick())
	{
		m_OverrideEmoteReset = -1;
	}

	if(m_Halloween && m_pCharacter && !m_pCharacter->IsPaused())
	{
		if(1200 - ((Server()->Tick() - m_pCharacter->GetLastAction()) % (1200)) < 5)
		{
			GameServer()->SendEmoticon(GetCid(), EMOTICON_GHOST, GetCid());
		}
	}
}

void CPlayer::ProcessStatsResult(CInstaSqlResult &Result)
{
	if(Result.m_Success) // SQL request was successful
	{
		switch(Result.m_MessageKind)
		{
		case CInstaSqlResult::DIRECT:
			for(auto &aMessage : Result.m_aaMessages)
			{
				if(aMessage[0] == 0)
					break;
				GameServer()->SendChatTarget(m_ClientId, aMessage);
			}
			break;
		case CInstaSqlResult::ALL:
		{
			bool PrimaryMessage = true;
			for(auto &aMessage : Result.m_aaMessages)
			{
				if(aMessage[0] == 0)
					break;

				if(GameServer()->ProcessSpamProtection(m_ClientId) && PrimaryMessage)
					break;

				GameServer()->SendChat(-1, TEAM_ALL, aMessage, -1);
				PrimaryMessage = false;
			}
			break;
		}
		case CInstaSqlResult::BROADCAST:
			// if(Result.m_aBroadcast[0] != 0)
			// 	GameServer()->SendBroadcast(Result.m_aBroadcast, -1);
			break;
		case CInstaSqlResult::STATS:
			GameServer()->m_pController->OnShowStatsAll(&Result.m_Stats, this, Result.m_Info.m_aRequestedPlayer);
			break;
		case CInstaSqlResult::RANK:
			GameServer()->m_pController->OnShowRank(Result.m_Rank, Result.m_RankedScore, Result.m_aRankColumnDisplay, this, Result.m_Info.m_aRequestedPlayer);
			break;
		case CInstaSqlResult::PLAYER_DATA:
			GameServer()->m_pController->OnLoadedNameStats(&Result.m_Stats, this);
			break;
		}
	}
}

int64_t CPlayer::HandleMulti()
{
	int64_t TimeNow = time_timestamp();
	if((TimeNow - m_LastKillTime) > 5)
	{
		m_Multi = 1;
		return TimeNow;
	}

	if(!GameServer()->m_pController->IsStatTrack())
		return TimeNow;

	m_Multi++;
	if(m_Stats.m_BestMulti < m_Multi)
		m_Stats.m_BestMulti = m_Multi;
	int Index = m_Multi - 2;
	m_Stats.m_aMultis[Index > MAX_MULTIS ? MAX_MULTIS : Index]++;

	if (!g_Config.m_TrainFngMode)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' multi x%d!",
			Server()->ClientName(GetCid()), m_Multi);
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	}
	return TimeNow;
}

void CPlayer::SetTeamSpoofed(int Team, bool DoChatMsg)
{
	KillCharacter();

	m_Team = Team;
	m_LastSetTeam = Server()->Tick();
	m_LastActionTick = Server()->Tick();
	m_SpectatorId = SPEC_FREEVIEW;

	protocol7::CNetMsg_Sv_Team Msg;
	Msg.m_ClientId = m_ClientId;
	Msg.m_Team = GameServer()->m_pController->GetPlayerTeam(this, true); // might be a fake team
	Msg.m_Silent = !DoChatMsg;
	Msg.m_CooldownTick = m_LastSetTeam + Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);

	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(auto &pPlayer : GameServer()->m_apPlayers)
		{
			if(pPlayer && pPlayer->m_SpectatorId == m_ClientId)
				pPlayer->m_SpectatorId = SPEC_FREEVIEW;
		}
	}

	Server()->ExpireServerInfo();
}

void CPlayer::SetTeamNoKill(int Team, bool DoChatMsg)
{
	m_Team = Team;
	m_LastSetTeam = Server()->Tick();
	m_LastActionTick = Server()->Tick();
	m_SpectatorId = SPEC_FREEVIEW;

	// dead spec mode for 0.7
	if(!m_IsDead)
	{
		protocol7::CNetMsg_Sv_Team Msg;
		Msg.m_ClientId = m_ClientId;
		Msg.m_Team = m_Team;
		Msg.m_Silent = !DoChatMsg;
		Msg.m_CooldownTick = m_LastSetTeam + Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
	}

	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(auto &pPlayer : GameServer()->m_apPlayers)
		{
			if(pPlayer && pPlayer->m_SpectatorId == m_ClientId)
				pPlayer->m_SpectatorId = SPEC_FREEVIEW;
		}
	}

	Server()->ExpireServerInfo();
}

void CPlayer::UpdateLastToucher(int ClientId)
{
	if(ClientId == GetCid())
		return;
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
	{
		// covers the reset case when -1 is passed explicitly
		// to reset the last toucher when being hammered by a team mate in fng
		m_LastToucherId = -1;
		return;
	}

	// TODO: should we really reset the last toucher when we get shot by a team mate?
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(
		pPlayer &&
		GameServer()->m_pController &&
		GameServer()->m_pController->IsTeamplay() &&
		pPlayer->GetTeam() == GetTeam())
	{
		m_LastToucherId = -1;
		return;
	}

	m_LastToucherId = ClientId;
}
