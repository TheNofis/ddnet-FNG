#ifndef GAME_SERVER_INSTAGIB_SQL_STATS_H
#define GAME_SERVER_INSTAGIB_SQL_STATS_H

#include <engine/server/databases/connection_pool.h>
#include <engine/shared/protocol.h>
#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/scoreworker.h>

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;

struct CInstaSqlResult : ISqlResult
{
	CInstaSqlResult();

	enum
	{
		MAX_MESSAGES = 10,
	};

	enum Variant
	{
		DIRECT,
		ALL,
		BROADCAST,
		// /statsall chat command
		STATS,
		// /rank_xxx chat command
		RANK,
		// initial stats load on connect
		PLAYER_DATA,
	} m_MessageKind;

	char m_aaMessages[MAX_MESSAGES][512];
	char m_aBroadcast[1024];
	struct
	{
		char m_aRequestedPlayer[MAX_NAME_LENGTH];
	} m_Info = {};

	CSqlStatsPlayer m_Stats;

	// if it is a /rank_* request the resulting rank will be in m_Rank
	int m_Rank = 0;

	// and the m_RankedScore is the amount it was ranked by
	// so this might be the amount of kills for /rank_kills
	int m_RankedScore = 0;

	// shown to the user
	char m_aRankColumnDisplay[128];

	// used as a sql table column
	char m_aRankColumnSql[128];

	void SetVariant(Variant v);
};

struct CSqlInstaData : ISqlData
{
	CSqlInstaData(std::shared_ptr<ISqlResult> pResult) :
		ISqlData(std::move(pResult))
	{
	}

	~CSqlInstaData() override;

	int m_DebugStats = 0;
	CExtraColumns *m_pExtraColumns = nullptr;
};

// read request
struct CSqlPlayerStatsRequest : CSqlInstaData
{
	CSqlPlayerStatsRequest(std::shared_ptr<CInstaSqlResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	// set to true when this is not a /stats chat command query
	// but the initial load of the players stats for save servers
	bool m_IsLoadPlayerData = false;

	// object being requested, player (16 bytes)
	char m_aName[MAX_NAME_LENGTH];
	char m_aRequestingPlayer[MAX_NAME_LENGTH];
	// relevant for /top5 kind of requests
	int m_Offset;

	// shown to the user
	char m_aRankColumnDisplay[128];

	// used as a sql table column
	char m_aRankColumnSql[128];

	// table name depends on gametype
	char m_aTable[128];

	// SQL ASC or DESC
	char m_aOrderBy[128];
};

// data to be writtem
struct CSqlPlayerFastcapData : CSqlInstaData
{
	CSqlPlayerFastcapData(std::shared_ptr<CInstaSqlResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	float m_Time;
	char m_aTimestamp[TIMESTAMP_STR_LENGTH];

	char m_aGameUuid[UUID_MAXSTRSIZE];

	// object being requested, player (16 bytes)
	char m_aName[MAX_NAME_LENGTH];
	char m_aRequestingPlayer[MAX_NAME_LENGTH];

	char m_aGametype[128];
	char m_aMap[128];
	bool m_Grenade = false;
	bool m_StatTrack = false;
};

// read request
struct CSqlPlayerFastcapRequest : CSqlInstaData
{
	CSqlPlayerFastcapRequest(std::shared_ptr<CInstaSqlResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	// object being requested, player (16 bytes)
	char m_aName[MAX_NAME_LENGTH];
	char m_aRequestingPlayer[MAX_NAME_LENGTH];
	// relevant for /top5 kind of requests
	int m_Offset;

	char m_aGametype[128];
	char m_aMap[128];
	bool m_Grenade = false;
	bool m_OnlyStatTrack = false;
};

// data to be written
struct CSqlSaveRoundStatsData : CSqlInstaData
{
	CSqlSaveRoundStatsData(int DebugStats) :
		CSqlInstaData(nullptr)
	{
		m_DebugStats = DebugStats;
	}

	char m_aName[128];
	char m_aTable[128];
	CSqlStatsPlayer m_Stats;
};

struct CSqlCreateTableRequest : ISqlData
{
	CSqlCreateTableRequest() :
		ISqlData(nullptr)
	{
	}
	char m_aName[128];
	char m_aColumns[2048];
};

class CSqlStats
{
	CDbConnectionPool *m_pPool;
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
	CGameContext *m_pGameServer;
	IServer *m_pServer;

	CExtraColumns *m_pExtraColumns = nullptr;

	// non ratelimited server side queries
	static bool CreateTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool CreateFastcapTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool SaveRoundStatsThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

	// ratelimited user queries

	static bool ShowStatsWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowFastcapRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowFastcapTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

	static bool SaveFastcapWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

	std::shared_ptr<CInstaSqlResult> NewInstaSqlResult(int ClientId);

	// Creates for player database requests
	void ExecPlayerStatsThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pName,
		const char *pTable,
		bool IsLoadPlayerData);

	void ExecPlayerRankOrTopThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pName,
		const char *pRankColumnDisplay,
		const char *pRankColumnSql,
		const char *pTable,
		const char *pOrderBy,
		int Offset);

	void ExecPlayerFastcapRankOrTopThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pName,
		const char *pMap,
		const char *pGametype,
		bool Grenade,
		bool OnlyStatTrack,
		int Offset);

	bool RateLimitPlayer(int ClientId);

public:
	CSqlStats(CGameContext *pGameServer, CDbConnectionPool *pPool);
	~CSqlStats() = default;

	void SetExtraColumns(CExtraColumns *pExtraColumns);

	void CreateTable(const char *pName);
	void CreateFastcapTable();
	void SaveRoundStats(const char *pName, const char *pTable, CSqlStatsPlayer *pStats);
	void SaveFastcap(int ClientId, int TimeTicks, const char *pTimestamp, bool Grenade, bool StatTrack);

	void LoadInstaPlayerData(int ClientId, const char *pTable);

	void ShowStats(int ClientId, const char *pName, const char *pTable);
	void ShowRank(int ClientId, const char *pName, const char *pRankColumnDisplay, const char *pRankColumnSql, const char *pTable, const char *pOrderBy);
	void ShowTop(int ClientId, const char *pName, const char *pRankColumnDisplay, const char *pRankColumnSql, const char *pTable, const char *pOrderBy, int Offset);
	void ShowFastcapRank(int ClientId, const char *pName, const char *pMap, const char *pGametype, bool Grenade, bool OnlyStatTrack);
	void ShowFastcapTop(int ClientId, const char *pName, const char *pMap, const char *pGametype, bool Grenade, bool OnlyStatTrack, int Offset);
};

#endif
