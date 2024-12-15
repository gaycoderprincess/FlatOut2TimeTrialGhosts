#include <windows.h>
#include <d3d9.h>
#include "toml++/toml.hpp"

#include "nya_dx9_hookbase.h"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "game.h"

#include "../nya-common-fouc/fo2versioncheck.h"

uintptr_t pControllerVTable = 0x67B920;
void SetPlayerControl(bool on) {
	NyaHookLib::Patch<uint32_t>(0x46F510, on ? 0x066C818B : 0xCC0004C2);
}

void SetGhostVisuals(bool on) {
	NyaHookLib::Patch<uint64_t>(0x43D8A3, on ? 0x266A000000B6840F : 0x266A00000000B7E9);
}

uintptr_t GetAnalogInput_call = 0x55EA50;
float __attribute__((naked)) __fastcall GetAnalogInput(Controller* pController, Controller::tInput* pInputStruct) {
	__asm__ (
		"mov eax, edx\n\t" // input struct ptr
		"mov edx, ecx\n\t" // controller ptr
		"call %0\n\t"
		"ret\n\t"
			:
			:  "m" (GetAnalogInput_call)
	);
}

#include "timetrialshared.h"

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

uintptr_t FinishLapASM_jmp = 0x47CDB1;
void __attribute__((naked)) FinishLapASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, ebx\n\t"
		"call %1\n\t"
		"popad\n\t"
		"mov ecx, 0x8E8410\n\t"
		"mov ecx, [ecx]\n\t"
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

void __fastcall SetAISameCar(Player* pPlayer) {
	pPlayer->nCarId = nPlayerCarID;

	if (pPlayer->nPlayerId <= 2) return;
	auto tmp = LoadTemporaryGhostForSpawning(pPlayer->nCarId);
	if (!tmp->IsValid()) return;
	if (tmp->sPlayerName[0] && pOpponentPlayerInfo) {
		wcscpy_s(pOpponentPlayerInfo->sPlayerName, 16, tmp->sPlayerName.c_str());
	}
	if (tmp->nCarSkinId < 1 || tmp->nCarSkinId > 5/*GetNumSkinsForCar(pPlayer->nCarId)*/) return;
	pPlayer->nCarSkinId = tmp->nCarSkinId;
}

uintptr_t AISameCarASM_jmp = 0x408A5D;
void __attribute__((naked)) AISameCarASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"
		"jmp %0\n\t"
			:
			:  "m" (AISameCarASM_jmp), "i" (SetAISameCar)
	);
}

void __attribute__((naked)) GetAINameASM() {
	__asm__ (
		"mov ecx, eax\n\t"
		"lea edx, [esi-0x1E]\n\t"
		"push ecx\n\t"
		"push edx\n\t"
		"push ebx\n\t"
		"push ebp\n\t"
		"push esi\n\t"
		"push edi\n\t"
		"call %0\n\t"
		"pop edi\n\t"
		"pop esi\n\t"
		"pop ebp\n\t"
		"pop ebx\n\t"
		"pop edx\n\t"
		"pop ecx\n\t"
		"ret\n\t"
			:
			:  "i" (GetAIName)
	);
}

void UpdateD3DProperties() {
	g_pd3dDevice = *(IDirect3DDevice9**)0x8DA788;
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
			DoFlatOutVersionCheck(FO2Version::FO2_1_2);

			InitAndReadConfigFile();

			if (bTimeTrialsEnabled) {
				ProcessGhostCarsASM_call = NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x409440, &ProcessGhostCarsASM);
				ProcessPlayerCarsASM_call = NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x46D5E9, &ProcessPlayerCarsASM);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x45D9C7, &GetAINameASM);
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x47CDAB, &FinishLapASM);
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x408A54, &AISameCarASM);
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x45DBFF, &GetPlayerCarASM);
				NyaHookLib::Fill(0x4DE31D, 0x90, 16);
				NyaHookLib::Patch<uint8_t>(0x45CD01 + 1, 2); // only spawn two ai
				NyaHookLib::Patch<uint8_t>(0x46C89A, 0xEB); // allow ai to ghost
				NyaHookLib::Patch<uint8_t>(0x430FC1, 0xEB); // use regular skins for ai
				NyaHookLib::Patch<uint8_t>(0x42FCB2, 0xEB); // use regular skins for ai

				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x409520, 0x40995E); // disable ai control
				if (nGhostVisuals == 0) SetGhostVisuals(false);
				if (bNoProps) {
					NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x59438C, 0x594421);
					NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x593BD0, 0x593C6F);
				}
			}
			else {
				bPBTimeDisplayEnabled = false;
				bCurrentSessionPBTimeDisplayEnabled = false;
				bViewReplayMode = false;
			}

			if (bShowInputsWhileDriving || bViewReplayMode || bPBTimeDisplayEnabled || bCurrentSessionPBTimeDisplayEnabled) {
				EndSceneOrig = (HRESULT(__thiscall*)(void*))(*(uintptr_t*)0x67D5A4);
				NyaHookLib::Patch(0x67D5A4, &EndSceneHook);
				D3DResetOrig = (void(*)())NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5A621D, &D3DResetHook);
			}
		} break;
		default:
			break;
	}
	return TRUE;
}