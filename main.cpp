#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "toml++/toml.hpp"

#include "nya_dx9_hookbase.h"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "game.h"

const int nLocalReplayVersion = 3;

enum eNitroType {
	NITRO_NONE,
	NITRO_FULL,
	NITRO_DOUBLE,
	NITRO_INFINITE,
};
int nNitroType = NITRO_FULL;
int nGhostVisuals = 2;
bool bNoProps = false;
bool bViewReplayMode = false;
bool bShowInputsWhileDriving = false;

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

struct tInputState {
	uint8_t keys[20];
};

std::vector<tCarState> aRecordingGhost;
std::vector<tInputState> aRecordingInputs;

struct tGhostSetup {
	std::vector<tCarState> aPBGhost;
	std::vector<tInputState> aPBInputs;
	uint32_t nPBTime = UINT_MAX;
};
tGhostSetup RollingLapPB;
tGhostSetup StartingLapPB;

bool bGhostLoaded = false;
bool bIsFirstLap = false;
double fGhostTime = 0;
double fGhostRecordTotalTime = 0;

bool IsPlayerStaging(Player* pPlayer) {
	return (pPlayer->nSomeFlags & 2) != 0;
}

bool ShouldGhostRun() {
	auto localPlayer = GetPlayer(0);
	if (!localPlayer) return false;
	if (!GetPlayer(1)) return false;
	if (IsPlayerStaging(localPlayer)) return false;
	return true;
}

std::string GetGhostFilename(int car, int track, bool isFirstLap) {
	auto path = "Ghosts/Track" + std::to_string(track) + "_Car" + std::to_string(car + 1);
	if (bNoProps) path += "_noprops";
	if (isFirstLap) path += "_lap1";
	switch (nNitroType) {
		case NITRO_NONE:
			path += "_nonitro";
			break;
		case NITRO_DOUBLE:
			path += "_2xnitro";
			break;
		case NITRO_INFINITE:
			path += "_infnitro";
			break;
		default:
			break;
	}
	path += ".fo2replay";
	return path;
}

void SavePB(tGhostSetup* ghost, int car, int track, bool isFirstLap) {
	std::filesystem::create_directory("Ghosts");

	auto fileName = GetGhostFilename(car, track, isFirstLap);
	auto outFile = std::ofstream(fileName, std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		WriteLog("Failed to save " + fileName + "!");
		return;
	}

	char signature[4] = "nya";
	int fileVersion = nLocalReplayVersion;

	size_t size = sizeof(tCarState);
	outFile.write(signature, sizeof(signature));
	outFile.write((char*)&fileVersion, sizeof(fileVersion));
	outFile.write((char*)&size, sizeof(size));
	outFile.write((char*)&car, sizeof(car));
	outFile.write((char*)&track, sizeof(track));
	outFile.write((char*)&ghost->nPBTime, sizeof(ghost->nPBTime));
	outFile.write((char*)&nNitroType, sizeof(nNitroType));
	outFile.write((char*)&isFirstLap, sizeof(isFirstLap));
	outFile.write((char*)&bNoProps, sizeof(bNoProps));
	int count = ghost->aPBGhost.size();
	outFile.write((char*)&count, sizeof(count));
	outFile.write((char*)&ghost->aPBGhost[0], sizeof(ghost->aPBGhost[0]) * count);
	outFile.write((char*)&ghost->aPBInputs[0], sizeof(ghost->aPBInputs[0]) * count);
}

void LoadPB(tGhostSetup* ghost, int car, int track, bool isFirstLap) {
	ghost->aPBGhost.clear();
	ghost->aPBInputs.clear();
	ghost->nPBTime = INT_MAX;

	auto fileName = GetGhostFilename(car, track, isFirstLap);
	auto inFile = std::ifstream(fileName, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) {
		WriteLog("No ghost found for " + fileName);
		return;
	}

	int tmpsize;
	int fileVersion;

	uint32_t signatureOrSize = 0;
	inFile.read((char*)&signatureOrSize, sizeof(signatureOrSize));
	if (memcmp(&signatureOrSize, "nya", 4) == 0) {
		inFile.read((char*)&fileVersion, sizeof(fileVersion));
		inFile.read((char*)&tmpsize, sizeof(tmpsize));
	}
	else {
		fileVersion = 1;
		tmpsize = signatureOrSize;
	}

	if (fileVersion > nLocalReplayVersion) {
		WriteLog("Too new ghost for " + fileName);
		return;
	}

	int tmpcar, tmptrack, tmptime;
	bool tmpfirstlap = false;
	bool tmpnoprops = false;
	int tmpnitro = NITRO_FULL;
	inFile.read((char*)&tmpcar, sizeof(tmpcar));
	inFile.read((char*)&tmptrack, sizeof(tmptrack));
	inFile.read((char*)&tmptime, sizeof(tmptime));
	if (fileVersion >= 2) {
		inFile.read((char*)&tmpnitro, sizeof(tmpnitro));
		inFile.read((char*)&tmpfirstlap, sizeof(tmpfirstlap));
		inFile.read((char*)&tmpnoprops, sizeof(tmpnoprops));
	}
	if (tmpsize != sizeof(tCarState)) {
		WriteLog("Outdated ghost for " + fileName);
		return;
	}
	if (tmpcar != car || tmptrack != track || tmpfirstlap != isFirstLap || tmpnoprops != bNoProps || tmpnitro != nNitroType) {
		WriteLog("Mismatched ghost for " + fileName);
		return;
	}
	ghost->nPBTime = tmptime;
	int count = 0;
	inFile.read((char*)&count, sizeof(count));
	ghost->aPBGhost.reserve(count);
	for (int i = 0; i < count; i++) {
		tCarState state;
		inFile.read((char*)&state, sizeof(state));
		ghost->aPBGhost.push_back(state);
	}
	if (fileVersion >= 3) {
		ghost->aPBInputs.reserve(count);
		for (int i = 0; i < count; i++) {
			tInputState state;
			inFile.read((char*)&state, sizeof(state));
			ghost->aPBInputs.push_back(state);
		}
	}
}

void ResetAndLoadPBGhost() {
	LoadPB(&StartingLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, true);
	LoadPB(&RollingLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, false);
	bGhostLoaded = true;
}

void InvalidateGhost() {
	bIsFirstLap = true;
	bGhostLoaded = false;
	fGhostTime = 0;
	fGhostRecordTotalTime = 0;
	RollingLapPB.nPBTime = UINT_MAX;
	RollingLapPB.aPBGhost.clear();
	RollingLapPB.aPBInputs.clear();
	StartingLapPB.nPBTime = UINT_MAX;
	StartingLapPB.aPBGhost.clear();
	StartingLapPB.aPBInputs.clear();
	aRecordingGhost.clear();
	aRecordingInputs.clear();
}

int GetCurrentPlaybackTick(tGhostSetup* ghost) {
	double currTime = fGhostTime;
	int tick = std::floor(100 * currTime);
	if (tick > ghost->aPBGhost.size() - 1) tick = ghost->aPBGhost.size() - 1;
	return tick;
}

void SetPlayerControl(bool on) {
	NyaHookLib::Patch<uint32_t>(0x46F510, on ? 0x066C818B : 0xCC0004C2);
}

void RunGhost(Player* pPlayer) {
	if (!pPlayer) return;

	int eventProperties[] = {6036, 0, 0, 0, 1000};
	pPlayer->TriggerEvent(eventProperties);

	fGhostTime += 0.01;

	auto ghost = bIsFirstLap ? &StartingLapPB : &RollingLapPB;

	if (ghost->aPBGhost.empty()) {
		if (!bViewReplayMode) pPlayer->pCar->mMatrix.a4 = {0,-25,0};
		return;
	}

	if (bViewReplayMode) SetPlayerControl(false);
	ghost->aPBGhost[GetCurrentPlaybackTick(ghost)].Apply(pPlayer);
}

uintptr_t GetAnalogInput_call = 0x55EA50;
float __attribute__((naked)) __fastcall GetAnalogInputASM(Controller* pController, Controller::tInput* pInputStruct) {
	__asm__ (
		"mov eax, edx\n\t" // input struct ptr
		"mov edx, ecx\n\t" // controller ptr
		"call %0\n\t"
		"ret\n\t"
			:
			:  "m" (GetAnalogInput_call)
	);
}

tInputState RecordInputs(Player* pPlayer) {
	static tInputState inputState;
	memset(&inputState, 0, sizeof(inputState));
	if (pPlayer->pController->_4[-1] == 0x67B920) {
		for (int i = 0; i < 20; i++) {
			auto value =  GetAnalogInputASM(pPlayer->pController, &pPlayer->pController->aInputs[i]) * 128;
			if (value < 0) inputState.keys[i] = 0;
			else inputState.keys[i] = value;
		}
	}
	else {
		for (int i = 0; i < 20; i++) {
			inputState.keys[i] = pPlayer->pController->GetInputValue(i);
		}
	}
	return inputState;
}

void RecordGhost(Player* pPlayer) {
	if (!pPlayer) return;
	if (!pPlayer->pController) return;

	fGhostRecordTotalTime += 0.01;

	tCarState state;
	state.Collect(pPlayer);
	aRecordingGhost.push_back(state);
	aRecordingInputs.push_back(RecordInputs(pPlayer));
}

void SetGhostVisuals(bool on) {
	NyaHookLib::Patch<uint64_t>(0x43D8A3, on ? 0x266A000000B6840F : 0x266A00000000B7E9);
}

void __fastcall ProcessGhostCar(Player* pPlayer) {
	if (!pPlayer) return;

	if (bViewReplayMode) SetPlayerControl(true);

	auto localPlayer = GetPlayer(0);
	auto ghostPlayer = GetPlayer(1);
	if (!localPlayer || !ghostPlayer) return;

	switch (nNitroType) {
		case NITRO_NONE:
			pGame->fNitroMultiplier = 0;
			break;
		case NITRO_FULL:
			pGame->fNitroMultiplier = 1;
			break;
		case NITRO_DOUBLE:
			pGame->fNitroMultiplier = 2;
			break;
		case NITRO_INFINITE:
			pGame->fNitroMultiplier = 50;
			break;
		default:
			break;
	}

	if (pPlayer == localPlayer) {
		if (nNitroType == NITRO_NONE) pPlayer->pCar->fNitro = 0;
		if (nNitroType == NITRO_INFINITE) pPlayer->pCar->fNitro = 10;
	}

	if (nGhostVisuals == 2 && pPlayer == ghostPlayer) {
		auto localPlayerPos = localPlayer->pCar->mMatrix.a4;
		auto ghostPlayerPos = ghostPlayer->pCar->mMatrix.a4;
		SetGhostVisuals((localPlayerPos - ghostPlayerPos).length() < 8);
	}

	if (IsPlayerStaging(localPlayer)) {
		InvalidateGhost();
	}

	if (ShouldGhostRun()) {
		if (!bGhostLoaded) ResetAndLoadPBGhost();

		if (bViewReplayMode) {
			if (pPlayer == localPlayer) RunGhost(pPlayer);
			if (pPlayer == ghostPlayer) {
				pPlayer->pCar->mMatrix.a4 = {0,-25,0};
				pPlayer->pCar->vVelocity = {0,0,0};
				pPlayer->pCar->vAngVelocity = {0,0,0};
				pPlayer->pCar->fGasPedal = 0;
				pPlayer->pCar->fBrakePedal = 0;
				pPlayer->pCar->fHandbrake = 1;
			}
		}
		else {
			if (pPlayer == ghostPlayer) RunGhost(pPlayer);
			if (pPlayer == localPlayer) RecordGhost(pPlayer);
		}
	}
	else if (pPlayer == ghostPlayer) {
		pPlayer->pCar->vVelocity = {0,0,0};
		pPlayer->pCar->vAngVelocity = {0,0,0};
		pPlayer->pCar->fGasPedal = 0;
		pPlayer->pCar->fBrakePedal = 0;
		pPlayer->pCar->fHandbrake = 1;
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
	auto ghost = bIsFirstLap ? &StartingLapPB : &RollingLapPB;
	if (!bViewReplayMode && lapTime < ghost->nPBTime) {
		WriteLog("Saving new lap PB of " + std::to_string(lapTime) + "ms");
		ghost->aPBGhost = aRecordingGhost;
		ghost->aPBInputs = aRecordingInputs;
		ghost->nPBTime = lapTime;
		SavePB(ghost, GetPlayer(0)->nCarId, pGame->nLevelId, bIsFirstLap);
	}
	bIsFirstLap = false;
	aRecordingGhost.clear();
	aRecordingInputs.clear();
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

auto gInputRGBBackground = NyaDrawing::CNyaRGBA32(215,215,215,255);
auto gInputRGBHighlight = NyaDrawing::CNyaRGBA32(0,255,0,255);

void DrawInputTriangle(float posX, float posY, float sizeX, float sizeY, float inputValue, bool invertValue) {
	float minX = std::min(posX - sizeX, posX + sizeX);
	float maxX = std::max(posX - sizeX, posX + sizeX);

	DrawTriangle(posX, posY - sizeY, posX - sizeX, std::lerp(posY - sizeY, posY + sizeY, 0.5), posX,
				 posY + sizeY, invertValue ? gInputRGBBackground : gInputRGBHighlight);

	DrawTriangle(posX, posY - sizeY, posX - sizeX, std::lerp(posY - sizeY, posY + sizeY, 0.5), posX,
				 posY + sizeY, invertValue ? gInputRGBHighlight : gInputRGBBackground, lerp(minX, maxX, inputValue), 0, 1, 1);
}

void DrawInputTriangleY(float posX, float posY, float sizeX, float sizeY, float inputValue, bool invertValue) {
	float minY = std::min(posY - sizeY, posY + sizeY);
	float maxY = std::max(posY - sizeY, posY + sizeY);

	DrawTriangle(std::lerp(posX - sizeX, posX + sizeX, 0.5), posY - sizeY, posX - sizeX, posY + sizeY, posX + sizeX,
				 posY + sizeY, invertValue ? gInputRGBBackground : gInputRGBHighlight);

	DrawTriangle(std::lerp(posX - sizeX, posX + sizeX, 0.5), posY - sizeY, posX - sizeX, posY + sizeY, posX + sizeX,
				 posY + sizeY, invertValue ? gInputRGBHighlight : gInputRGBBackground, 0, lerp(minY, maxY + 0.001, inputValue), 1, 1);
}

void DrawInputRectangle(float posX, float posY, float scaleX, float scaleY, float inputValue) {
	DrawRectangle(posX - scaleX, posX + scaleX, posY - scaleY, posY + scaleY, gInputRGBBackground);
	DrawRectangle(posX - scaleX, posX + scaleX, lerp(posY + scaleY, posY - scaleY, inputValue), posY + scaleY, gInputRGBHighlight);
}

void DisplayInputs(tInputState* inputs) {
	float fBaseXPosition = 0.2;
	float fBaseYPosition = 0.85;

	DrawInputTriangle((fBaseXPosition - 0.005) * GetAspectRatioInv(), fBaseYPosition, 0.08 * GetAspectRatioInv(), 0.07, 1 - (inputs->keys[INPUT_STEER_LEFT] / 128.0), true);
	DrawInputTriangle((fBaseXPosition + 0.08) * GetAspectRatioInv(), fBaseYPosition, -0.08 * GetAspectRatioInv(), 0.07, inputs->keys[INPUT_STEER_RIGHT] / 128.0, false);
	DrawInputTriangleY((fBaseXPosition + 0.0375) * GetAspectRatioInv(), fBaseYPosition - 0.05, 0.035 * GetAspectRatioInv(), 0.045, 1 - (inputs->keys[INPUT_ACCELERATE] / 128.0), true);
	DrawInputTriangleY((fBaseXPosition + 0.0375) * GetAspectRatioInv(), fBaseYPosition + 0.05, 0.035 * GetAspectRatioInv(), -0.045, inputs->keys[INPUT_BRAKE] / 128.0, false);

	DrawInputTriangleY((fBaseXPosition + 0.225) * GetAspectRatioInv(), fBaseYPosition - 0.04, 0.035 * GetAspectRatioInv(), 0.035, 1 - (inputs->keys[INPUT_GEAR_UP] / 128.0), true);
	DrawInputTriangleY((fBaseXPosition + 0.225) * GetAspectRatioInv(), fBaseYPosition + 0.04, 0.035 * GetAspectRatioInv(), -0.035, inputs->keys[INPUT_GEAR_DOWN] / 128.0, false);

	DrawInputRectangle((fBaseXPosition + 0.325) * GetAspectRatioInv(), fBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->keys[INPUT_NITRO] / 128.0);
	DrawInputRectangle((fBaseXPosition + 0.425) * GetAspectRatioInv(), fBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->keys[INPUT_HANDBRAKE] / 128.0);
	DrawInputRectangle((fBaseXPosition + 0.525) * GetAspectRatioInv(), fBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->keys[INPUT_RESET] / 128.0);
}

void HookLoop() {
	if (auto player = GetPlayer(0)) {
		if (bViewReplayMode) {
			auto ghost = bIsFirstLap ? &StartingLapPB : &RollingLapPB;
			if (!ghost->aPBInputs.empty()) {
				DisplayInputs(&ghost->aPBInputs[GetCurrentPlaybackTick(ghost)]);
			}
		}
		else if (bShowInputsWhileDriving) {
			auto inputs = RecordInputs(player);
			DisplayInputs(&inputs);
		}
	}

	CommonMain();
}

void UpdateD3DProperties() {
	g_pd3dDevice = *(IDirect3DDevice9 **) 0x8DA788;
	ghWnd = *(HWND*)0x8DA79C;
	nResX = *(int*)0x6D68E8;
	nResY = *(int*)0x6D68EC;
}

bool bDeviceJustReset = false;
void D3DHookMain() {
	if (!g_pd3dDevice) {
		UpdateD3DProperties();
		InitHookBase();
	}

	if (bDeviceJustReset) {
		ImGui_ImplDX9_CreateDeviceObjects();
		bDeviceJustReset = false;
	}
	HookBaseLoop();
}

auto EndSceneOrig = (HRESULT(__thiscall*)(void*))nullptr;
HRESULT __fastcall EndSceneHook(void* a1) {
	D3DHookMain();
	return EndSceneOrig(a1);
}

auto D3DResetOrig = (void(*)())nullptr;
void D3DResetHook() {
	if (g_pd3dDevice) {
		UpdateD3DProperties();
		ImGui_ImplDX9_InvalidateDeviceObjects();
		bDeviceJustReset = true;
	}
	return D3DResetOrig();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x202638) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.2 (.exe size of 2990080 bytes)", "nya?!~", MB_ICONERROR);
				exit(0);
				return TRUE;
			}

			// at 30fps this should be a few minutes?
			aRecordingGhost.reserve(10000);
			aRecordingInputs.reserve(10000);

			auto config = toml::parse_file("FlatOut2TimeTrialGhosts_gcp.toml");
			nGhostVisuals = config["main"]["ghost_visuals"].value_or(2);
			bNoProps = config["main"]["disable_props"].value_or(false);
			nNitroType = config["main"]["nitro_option"].value_or(NITRO_FULL);
			bViewReplayMode = config["extras"]["view_replays"].value_or(false);
			bShowInputsWhileDriving = config["extras"]["always_show_input_display"].value_or(false);
			gInputRGBHighlight.r = config["input_display"]["highlight_r"].value_or(0);
			gInputRGBHighlight.g = config["input_display"]["highlight_g"].value_or(255);
			gInputRGBHighlight.b = config["input_display"]["highlight_b"].value_or(0);
			gInputRGBBackground.r = config["input_display"]["background_r"].value_or(200);
			gInputRGBBackground.g = config["input_display"]["background_g"].value_or(200);
			gInputRGBBackground.b = config["input_display"]["background_b"].value_or(200);

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

			if (bShowInputsWhileDriving || bViewReplayMode) {
				EndSceneOrig = (HRESULT(__thiscall *)(void *)) (*(uintptr_t *) 0x67D5A4);
				NyaHookLib::Patch(0x67D5A4, &EndSceneHook);
				D3DResetOrig = (void (*)()) NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5A621D, &D3DResetHook);
			}

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x409520, 0x40995E); // disable ai control
			if (nGhostVisuals == 0) SetGhostVisuals(false);
			if (bNoProps) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x59438C, 0x594421);
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x593BD0, 0x593C6F);
			}
		} break;
		default:
			break;
	}
	return TRUE;
}