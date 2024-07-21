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
	tMatrix mMatrix;			// +1B0
	uint8_t _1F0[0x80];
	float qQuaternion[4];		// +270
	NyaVec3 vVelocity;			// +280
	uint8_t _28C[0x4];
	NyaVec3 vAngVelocity;		// +290
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
};

class Player {
public:
	uint8_t _4[0x338];
	Car* pCar;					// +33C
	uint32_t nCarId;			// +340
	uint32_t nSkinId;			// +344
	uint8_t _348[0x2C];
	uint32_t nStagingEngineRev;	// +374

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

class PlayerHost {
public:
	uint8_t _0[0x14];
	Player** aPlayers;
};

class tGameMain {
public:
	uint8_t _0[0x480];
	int nLevelId;			// +480
	uint8_t _484[0x4];
	int nNumLaps;			// +488
	uint8_t _48C[0x52C];
	PlayerHost* pHost;		// +9B8
};
auto& pGame = *(tGameMain**)0x8E8410;

Player* GetPlayer(int id) {
	auto host = pGame->pHost;
	if (!host) return nullptr;
	auto players = host->aPlayers;
	if (!players) return nullptr;
	auto ply = players[id];
	if (!ply || !ply->pCar) return nullptr;
	return ply;
}