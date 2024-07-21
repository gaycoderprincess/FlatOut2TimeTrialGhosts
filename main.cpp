#include <windows.h>
#include "toml++/toml.hpp"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "game.h"

int nGhostVisuals = 2;

void WriteLog(const std::string& str) {
	static auto file = std::ofstream("FlatOut2TimeTrialGhosts_gcp.log");

	file << str;
	file << "\n";
	file.flush();
}

struct tCarState {
	tMatrix mat;
	NyaVec3 vel;
	NyaVec3 tvel;
	float quat[4];
	float nitro;
	float damage;
	float steer;
	float gas;
	float brake;
	float nitroHeld;
	float handbrake;
	int gear;

	void Collect(Player* pPlayer) {
		auto car = pPlayer->pCar;
		mat.a1 = car->mMatrix.a1;
		mat.a2 = car->mMatrix.a2;
		mat.a3 = car->mMatrix.a3;
		mat.a4 = car->mMatrix.a4;
		vel = car->vVelocity;
		tvel = car->vAngVelocity;
		memcpy(quat, car->qQuaternion, sizeof(quat));
		nitro = car->fNitro;
		damage = car->fDamage;
		steer = car->fSteerAngle;
		gas = car->fGasPedal;
		brake = car->fBrakePedal;
		nitroHeld = car->fNitroButton;
		handbrake = car->fHandbrake;
		gear = car->mGearbox.nGear;
	}

	void Apply(Player* pPlayer) {
		auto car = pPlayer->pCar;
		car->mMatrix.a1 = mat.a1;
		car->mMatrix.a2 = mat.a2;
		car->mMatrix.a3 = mat.a3;
		car->mMatrix.a4 = mat.a4;
		car->vVelocity = vel;
		car->vAngVelocity = tvel;
		memcpy(car->qQuaternion, quat, sizeof(quat));
		car->fNitro = nitro;
		car->fDamage = damage;
		car->fSteerAngle = steer;
		car->fGasPedal = gas;
		car->fBrakePedal = brake;
		car->fNitroButton = nitroHeld;
		car->fHandbrake = handbrake;
		car->mGearbox.nGear = gear;
	}
};
std::vector<tCarState> aRecordingGhost;
std::vector<tCarState> aPBGhost;

bool bGhostLoaded = false;
int nPBTime = INT_MAX;
double fGhostTime = 0;
double fGhostRecordTotalTime = 0;

bool ShouldGhostRun() {
	auto localPlayer = GetPlayer(0);
	if (!localPlayer) return false;
	if (!GetPlayer(1)) return false;
	if (localPlayer->nStagingEngineRev) return false;
	return true;
}

std::string GetGhostFilename(int car, int track) {
	return "Ghosts/Track" + std::to_string(track) + "_Car" + std::to_string(car + 1) + ".fo2replay";
}

void SavePB(int car, int track) {
	std::filesystem::create_directory("Ghosts");

	auto fileName = GetGhostFilename(car, track);
	auto outFile = std::ofstream(fileName, std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		WriteLog("Failed to save " + fileName + "!");
		return;
	}

	size_t size = sizeof(tCarState);
	outFile.write((char*)&size, sizeof(size));
	outFile.write((char*)&car, sizeof(car));
	outFile.write((char*)&track, sizeof(track));
	outFile.write((char*)&nPBTime, sizeof(nPBTime));
	int count = aPBGhost.size();
	outFile.write((char*)&count, sizeof(count));
	outFile.write((char*)&aPBGhost[0], sizeof(aPBGhost[0]) * count);
}

void LoadPB(int car, int track) {
	aPBGhost.clear();
	nPBTime = INT_MAX;

	auto fileName = GetGhostFilename(car, track);
	auto inFile = std::ifstream(fileName, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) {
		WriteLog("No ghost found for " + fileName);
		return;
	}

	int tmpcar, tmptrack, tmpsize, tmptime;
	inFile.read((char*)&tmpsize, sizeof(tmpsize));
	inFile.read((char*)&tmpcar, sizeof(tmpcar));
	inFile.read((char*)&tmptrack, sizeof(tmptrack));
	inFile.read((char*)&tmptime, sizeof(tmptime));
	if (tmpsize != sizeof(tCarState)) {
		WriteLog("Outdated ghost for " + fileName);
		return;
	}
	if (tmpcar != car || tmptrack != track) {
		WriteLog("Mismatched ghost for " + fileName);
		return;
	}
	nPBTime = tmptime;
	int count = 0;
	inFile.read((char*)&count, sizeof(count));
	aPBGhost.reserve(count);
	for (int i = 0; i < count; i++) {
		tCarState state;
		inFile.read((char*)&state, sizeof(state));
		aPBGhost.push_back(state);
	}
}

void ResetAndLoadPBGhost() {
	LoadPB(GetPlayer(0)->nCarId, pGame->nLevelId);
	bGhostLoaded = true;
}

void InvalidateGhost() {
	bGhostLoaded = false;
	fGhostTime = 0;
	fGhostRecordTotalTime = 0;
	nPBTime = INT_MAX;
	aRecordingGhost.clear();
	aPBGhost.clear();
}

void RunGhost(Player* pPlayer) {
	if (!pPlayer) return;

	int eventProperties[] = {6036, 0, 0, 0, 100};
	pPlayer->TriggerEvent(eventProperties);

	fGhostTime += 0.01;

	if (aPBGhost.empty()) {
		pPlayer->pCar->mMatrix.a4 = {0,-25,0};
		return;
	}

	double currTime = fGhostTime;
	int tick = std::floor(100 * currTime);

	if (tick > aPBGhost.size() - 1) tick = aPBGhost.size() - 1;
	aPBGhost[tick].Apply(pPlayer);
}

void RecordGhost(Player* pPlayer) {
	if (!pPlayer) return;

	fGhostRecordTotalTime += 0.01;

	tCarState state;
	state.Collect(pPlayer);
	aRecordingGhost.push_back(state);
}

void SetGhostVisuals(bool on) {
	NyaHookLib::Patch<uint64_t>(0x43D8A3, on ? 0x266A000000B6840F : 0x266A00000000B7E9);
}

void __fastcall ProcessGhostCar(Player* pPlayer) {
	if (!pPlayer) return;

	auto localPlayer = GetPlayer(0);
	auto ghostPlayer = GetPlayer(1);
	if (!localPlayer || !ghostPlayer) return;

	if (nGhostVisuals == 2) {
		auto localPlayerPos = localPlayer->pCar->mMatrix.a4;
		auto ghostPlayerPos = ghostPlayer->pCar->mMatrix.a4;
		SetGhostVisuals((localPlayerPos - ghostPlayerPos).length() < 8);
	}

	if (localPlayer->nStagingEngineRev > 0) {
		InvalidateGhost();
	}

	if (ShouldGhostRun()) {
		if (!bGhostLoaded) ResetAndLoadPBGhost();

		if (pPlayer == ghostPlayer) RunGhost(pPlayer);
		if (pPlayer == localPlayer) RecordGhost(pPlayer);
	}
	else if (pPlayer == ghostPlayer) {
		ghostPlayer->pCar->vVelocity = {0,0,0};
		ghostPlayer->pCar->vAngVelocity = {0,0,0};
		ghostPlayer->pCar->fGasPedal = 0;
		ghostPlayer->pCar->fBrakePedal = 0;
		ghostPlayer->pCar->fHandbrake = 1;
	}
}

uintptr_t ProcessGhostCarsASM_call = 0x46C850;
void __attribute__((naked)) ProcessGhostCarsASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, eax\n\t"
		"call %1\n\t"
		"popad\n\t"
		"jmp %0\n\t"
			:
			:  "m" (ProcessGhostCarsASM_call), "i" (ProcessGhostCar)
	);
}

uintptr_t ProcessPlayerCarsASM_call = 0x46C850;
void __attribute__((naked)) ProcessPlayerCarsASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, eax\n\t"
		"call %1\n\t"
		"popad\n\t"
		"jmp %0\n\t"
			:
			:  "m" (ProcessPlayerCarsASM_call), "i" (ProcessGhostCar)
	);
}

void __fastcall OnFinishLap(uint32_t lapTime) {
	if (lapTime < nPBTime) {
		WriteLog("Saving new lap PB of " + std::to_string(lapTime) + "ms");
		aPBGhost = aRecordingGhost;
		nPBTime = lapTime;
		SavePB(GetPlayer(0)->nCarId, pGame->nLevelId);
	}
	aRecordingGhost.clear();
	fGhostTime = 0;
	fGhostRecordTotalTime = 0;
}

uintptr_t FinishLapASM_jmp = 0x47CDB1;
void __attribute__((naked)) FinishLapASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, ebx\n\t"
		"call %1\n\t"
		"popad\n\t"
		"mov ecx, 0x8E8410\n\t"
		"jmp %0\n\t"
			:
			:  "m" (FinishLapASM_jmp), "i" (OnFinishLap)
	);
}

int nPlayerCarID = 0;

uintptr_t GetPlayerCarASM_jmp = 0x45DC0A;
void __attribute__((naked)) GetPlayerCarASM() {
	__asm__ (
		"mov ecx, [esi-0x14]\n\t"
		"mov edx, [esi]\n\t"
		"mov %1, ecx\n\t"
		"mov [ebx+0x340], ecx\n\t"
		"jmp %0\n\t"
			:
			:  "m" (GetPlayerCarASM_jmp), "m" (nPlayerCarID)
	);
}

uintptr_t AISameCarASM_jmp = 0x408A5D;
void __attribute__((naked)) AISameCarASM() {
	__asm__ (
		"mov ecx, %1\n\t"
		"mov [esi+0x340], ecx\n\t"
		"jmp %0\n\t"
			:
			:  "m" (AISameCarASM_jmp), "m" (nPlayerCarID)
	);
}

const wchar_t* GetAIName() {
	return L"PB GHOST";
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x202638) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.2 (.exe size of 2990080 bytes)", "nya?!~", MB_ICONERROR);
				exit(0);
				return TRUE;
			}

			aRecordingGhost.reserve(10000); // at 30fps this should be a few minutes?

			auto config = toml::parse_file("FlatOut2TimeTrialGhosts_gcp.toml");
			nGhostVisuals = config["main"]["ghost_visuals"].value_or(2);

			ProcessGhostCarsASM_call = NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x409440, &ProcessGhostCarsASM);
			ProcessPlayerCarsASM_call = NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x46D5E9, &ProcessPlayerCarsASM);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x45D9C7, &GetAIName);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x47CDAB, &FinishLapASM);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x408A54, &AISameCarASM);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x45DBFF, &GetPlayerCarASM);
			NyaHookLib::Fill(0x4DE31D, 0x90, 16);
			NyaHookLib::Patch<uint8_t>(0x45CD01 + 1, 1); // only spawn one ai
			NyaHookLib::Patch<uint8_t>(0x46C89A, 0xEB); // allow ai to ghost
			NyaHookLib::Patch<uint8_t>(0x430FC1, 0xEB); // use regular skins for ai
			NyaHookLib::Patch<uint8_t>(0x42FCB2, 0xEB); // use regular skins for ai

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x409520, 0x40995E); // disable ai control
			if (nGhostVisuals == 0) SetGhostVisuals(false);
		} break;
		default:
			break;
	}
	return TRUE;
}