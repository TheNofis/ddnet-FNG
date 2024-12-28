/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_H
#define GAME_SERVER_ENTITIES_LASER_H

#include <game/server/entity.h>

class CLaserChar : public CEntity {
public:
	CLaserChar(CGameWorld *pGameWorld) : CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER) {}
	int getID() { return m_Id; }
	vec2 m_Frompos;
};

class CLaserText : public CEntity
{
public:
	CLaserText(CGameWorld *pGameWorld, vec2 Pos, int Owner, int pAliveTicks, char* pText, int pTextLen);
	CLaserText(CGameWorld *pGameWorld, vec2 Pos, int Owner, int pAliveTicks, char* pText, int pTextLen, float pCharPointOffset, float pCharOffsetFactor);
	~CLaserText() override{
		delete[] m_Text; 
		for(int i = 0; i < m_CharNum; ++i) {
			delete m_Chars[i];
		}
		delete[] m_Chars;
	}

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;

private:
	float m_PosOffsetCharPoints;
	float m_PosOffsetChars;

	void makeLaser(char pChar, int pCharOffset, int& CharCount);

	int m_Owner;
	
	int m_AliveTicks;
	int m_CurTicks;
	int m_StartTick;
	
	char* m_Text;
	int m_TextLen;
	
	CLaserChar** m_Chars;
	int m_CharNum;
};

#endif
