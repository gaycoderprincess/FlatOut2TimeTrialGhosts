struct tMatrix {
	NyaVec3 a1;
	float w1;
	NyaVec3 a2;
	float w2;
	NyaVec3 a3;
	float w3;
	NyaVec3 a4;
	float w4;
};

class Gearbox {
public:
	uint8_t _0[0x40];
	uint8_t nGear;
	uint8_t _41[0x83];
};

class Player;
class Car {
public:
	uint8_t _0[0x1B0];
	float mMatrix[4*4];			// +1B0
	uint8_t _1F0[0x80];
	float qQuaternion[4];		// +270
	float vVelocity[3];			// +280
	uint8_t _28C[0x4];
	float vAngVelocity[3];		// +290
	uint8_t _29C[0x330];
	float fNitro;				// +5CC
	uint8_t _5D0[0x24];
	Gearbox mGearbox;			// +5F4
	uint8_t _6B8[0x173C];
	float fGasPedal;			// +1DF4
	float fBrakePedal;			// +1DF8
	float fNitroButton;			// +1DFC
	float fHandbrake;			// +1E00
	float fSteerAngle;			// +1E04
	uint8_t _1E08[0x4C98];
	float fDamage;				// +6AA0

#ifdef NYA_MATH_H
	inline NyaMat4x4* GetMatrix() {
		return (NyaMat4x4*)mMatrix;
	}
	inline NyaVec3* GetVelocity() {
		return (NyaVec3*)vVelocity;
	}
	inline NyaVec3* GetAngVelocity() {
		return (NyaVec3*)vAngVelocity;
	}
#endif
};

class LiteDb {
public:

	virtual void _vf0() = 0;
	virtual void _vf1() = 0;
	virtual void _vf2() = 0;
	virtual void _vf3() = 0;
	virtual void _vf4() = 0;
	virtual void _vf5() = 0;
	virtual void _vf6() = 0;
	virtual void _vf7() = 0;
	virtual void _vf8() = 0;
	virtual void _vf9() = 0;
	virtual LiteDb* GetTable(const char* name) = 0;
	virtual void _vf11() = 0;
	virtual void _vf12() = 0;
	virtual void _vf13() = 0;
	virtual void _vf14() = 0;
	virtual void _vf15() = 0;
	virtual void _vf16() = 0;
	virtual void _vf17() = 0;
	virtual void _vf18() = 0;
	virtual void _vf19() = 0;
	virtual void _vf20() = 0;
	virtual void _vf21() = 0;
	virtual void _vf22() = 0;
	virtual void _vf23() = 0;
	virtual void _vf24() = 0;
	virtual void _vf25() = 0;
	virtual void _vf26() = 0;
	virtual void _vf27() = 0;
	virtual void _vf28() = 0;
	virtual void _vf29() = 0;
	virtual void _vf30() = 0;
	virtual void _vf31() = 0;
	virtual void _vf32() = 0;
	virtual void _vf33() = 0;
	virtual void _vf34() = 0;
	virtual void* GetPropertyPointer(const char* name) = 0;
	virtual int GetPropertyAsBool(const char* name, int offset) = 0;
	virtual int GetPropertyAsInt(const char* name, int offset) = 0;
	virtual char GetPropertyAsChar(const char* name, int offset) = 0;
	virtual float GetPropertyAsFloat(const char* name, int offset) = 0;
	virtual uint32_t GetPropertyAsRGBA(const char* name, int offset) = 0;
	virtual void* GetPropertyAsVector2(void* out, const char* name, int offset) = 0;
	virtual void* GetPropertyAsVector3(void* out, const char* name, int offset) = 0;
	virtual void* GetPropertyAsVector4(void* out, const char* name, int offset) = 0;
	virtual LiteDb* GetPropertyAsNode(void* out, const char* name, int offset) = 0;
	virtual LiteDb* GetPropertyAsNodePtr(const char* name, int offset) = 0;
	virtual const char* GetPropertyAsString(const char* name) = 0;
};
auto GetLiteDB = (LiteDb*(*)())0x54C960;

class ScriptHost {
public:
	struct tLUAStruct {
		const char* unkString; // "unnamed"
		void* pLUAContext; // +4
	};

	uint8_t _0[0x8];
	tLUAStruct* pLUAStruct;
};
auto& pScriptHost = *(ScriptHost**)0x8E8418;

enum eControllerInput {
	INPUT_HANDBRAKE = 0,
	INPUT_NITRO = 1,
	INPUT_CAMERA = 3,
	INPUT_LOOK_BEHIND = 4,
	INPUT_RESET = 5,
	INPUT_PLAYERLIST = 7,
	INPUT_MENU_ACCEPT = 8,
	INPUT_PAUSE = 9,
	INPUT_ACCELERATE = 10,
	INPUT_BRAKE = 11,
	INPUT_STEER_LEFT = 12,
	INPUT_STEER_RIGHT = 13,
	INPUT_GEAR_DOWN = 18,
	INPUT_GEAR_UP = 19,
};

class Controller {
public:
	uint32_t _4[1];
	uint8_t _8[0x63C];
	struct tInput {
		uint8_t _0[0x10];
	} aInputs[20];

	virtual void _vf0() = 0;
	virtual void _vf1() = 0;
	virtual void _vf2() = 0;
	virtual void _vf3() = 0;
	virtual void _vf4() = 0;
	virtual void _vf5() = 0;
	virtual void _vf6() = 0;
	virtual void _vf7() = 0;
	virtual void _vf8() = 0;
	virtual void _vf9() = 0;
	virtual void _vf10() = 0;
	virtual int GetInputValue(int input) = 0;
};

enum ePlayerEvent {
	PLAYEREVENT_RESPAWN_GHOST = 6036,
};

class Player {
public:
	uint8_t _4[0x328];
	Controller* pController;	// +32C
	uint8_t _330[0xC];
	Car* pCar;					// +33C
	uint32_t nCarId;			// +340
	uint32_t nCarSkinId;		// +344
	uint8_t _348[0x20];
	uint32_t nPlayerId;			// +368
	uint8_t _36C[0x8];
	uint32_t nStagingEngineRev;	// +374
	uint8_t _378[0x324];
	uint32_t nSomeFlags;		// +69C

	virtual void _vf0() = 0;
	virtual void _vf1() = 0;
	virtual void _vf2() = 0;
	virtual void _vf3() = 0;
	virtual void _vf4() = 0;
	virtual void _vf5() = 0;
	virtual void _vf6() = 0;
	virtual void _vf7() = 0;
	virtual void _vf8() = 0;
	virtual void _vf9() = 0;
	virtual void _vf10() = 0;
	virtual void _vf11() = 0;
	virtual void _vf12() = 0;
	virtual void _vf13() = 0;
	virtual void _vf14() = 0;
	virtual void _vf15() = 0;
	virtual void _vf16() = 0;
	virtual void _vf17() = 0;
	virtual void _vf18() = 0;
	virtual void _vf19() = 0;
	virtual void TriggerEvent(int* properties) = 0;
};

class PlayerInfo {
public:
	wchar_t sPlayerName[16]; // +0
	int nCar; // +20
	int nCarSkin; // +24
	int nCarUpgrades; // +28
	int nType; // +2C
	int nController; // +30
	int nCharType; // +34
	int nPeerId; // +38
	int nNetworkId; // +3C
	uint8_t _40[0x10]; // +40
};

class PlayerProfile {
public:
	uint8_t _0[0xE34];
	wchar_t sPlayerName[16]; // +E34
};

class PlayerHost {
public:
	uint8_t _0[0x14];
	Player** aPlayers;
};

class Game {
public:
	uint8_t _0[0x480];
	int nLevelId;			// +480
	uint8_t _484[0x4];
	int nNumLaps;			// +488
	uint8_t _48C[0x18];
	float fNitroMultiplier;	// +4A4
	uint8_t _4A8[0x510];
	PlayerHost* pHost;		// +9B8
	uint8_t _9BC[0x624];
	PlayerProfile Profile;	// +FE0
};
auto& pGame = *(Game**)0x8E8410;

class PlayerScoreRace {
public:
	uint8_t _0[0x4];
	uint32_t nPlayerId; // 4
	uint8_t _8[0x3C];
	uint32_t nCurrentLap; // 44
};

class ScoreManager {
public:
	uint8_t _0[0x8];
	void** pScoresStart;
	void** pScoresEnd;
};
auto& pScoreManager = *(ScoreManager**)0x8DA5D0;

auto& pLoadingScreen = *(void**)0x8E8448;

class Font {
public:
	uint8_t _0[0x4];
	NyaDrawing::CNyaRGBA32 nColor;
	uint8_t _8[0x2C];
	struct {
		bool bXCenterAlign : 1;
		bool bXRightAlign : 1;
	};
	uint8_t _38[0x14];
	float fScaleX;
	float fScaleY;

	static inline auto GetFont = (Font*(__stdcall*)(void*, const char*))0x451840;
	static inline auto Display = (void(*)(Font*, float, float, const wchar_t*, ...))0x54E820;
};

Player* GetPlayer(int id) {
	auto host = pGame->pHost;
	if (!host) return nullptr;
	auto players = host->aPlayers;
	if (!players) return nullptr;
	auto ply = players[id];
	if (!ply || !ply->pCar) return nullptr;
	return ply;
}

void DrawStringFO2(tNyaStringData data, const wchar_t* string, const char* font) {
	auto pFont = Font::GetFont(*(void**)(0x8E8434), font);
	pFont->fScaleX = data.size * nResX / 20.0;
	pFont->fScaleY = data.size * nResY / 20.0;
	pFont->bXRightAlign = data.XRightAlign;
	pFont->bXCenterAlign = data.XCenterAlign;
	pFont->nColor.r = data.r;
	pFont->nColor.g = data.g;
	pFont->nColor.b = data.b;
	pFont->nColor.a = data.a;
	pFont->fScaleX *= GetAspectRatioInv();
	Font::Display(pFont, data.x * nResX, data.y * nResY, string);
}

template<typename T>
T* GetPlayerScore(int playerId) {
	if (!pScoreManager) return nullptr;

	auto score = (T**)pScoreManager->pScoresStart;
	auto end = (T**)pScoreManager->pScoresEnd;
	while (score < end) {
		if ((*score)->nPlayerId + 1 == playerId) {
			return *score;
		}
		score++;
	}
	return nullptr;
}

auto luaL_checktype = (void(*)(void*, int, int))0x5B5DC0;
auto luaL_checkudata = (void*(*)(void*, int, const char*))0x5B5D00;
auto luaL_typerror = (void(*)(void*, int, const char*))0x5B5A50;
auto luaL_checknumber = (double(*)(void*, int))0x5B5F20;
auto lua_tonumber = (double(*)(void*, int))0x5B4300;
auto lua_setfield = (void(*)(void*, int, const char*))0x5B4E70;
auto lua_pushcfunction = (void(*)(void*, void*, int))0x5B48A0;
auto lua_settable = (void(*)(void*, int))0x5B4E20;
auto lua_setglobal = (void(*)(void*, const char*))0x5B4790;
auto lua_gettop = (int(*)(void*))0x5B3C70;
auto lua_rawgeti = (void(*)(void*, int, int))0x5B4BD0;
auto lua_tolstring = (const char*(*)(void*, int, void*))0x5B4400;
auto lua_getfield = (void(*)(void*, int, const char*))0x5B4AD0;
auto lua_settop = (void(*)(void*, int))0x5B3C90;
auto lua_gettable = (void(*)(void*, int))0x5B4A90;

const char* GetCarName(int id) {
	auto table = GetLiteDB()->GetTable(std::format("FlatOut2.Cars.Car[{}]", id).c_str());
	return (const char*)table->GetPropertyPointer("Name");
}

const char* GetTrackName(int id) {
	auto lua = pScriptHost->pLUAStruct->pLUAContext;
	auto oldtop = lua_gettop(lua);

	lua_getfield(lua, -10002, "Levels");
	lua_rawgeti(lua, lua_gettop(lua), id);

	auto oldtop2 = lua_gettop(lua);
	lua_setglobal(lua, "Name");
	lua_gettable(lua, oldtop2);
	auto name = (const char*)lua_tolstring(lua, lua_gettop(lua), 0);

	lua_settop(lua, oldtop);

	return name;
}