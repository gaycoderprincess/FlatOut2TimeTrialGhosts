const int nLocalReplayVersion = 4;

enum eLapType {
	LAPTYPE_ROLLING,
	LAPTYPE_STANDING,
	LAPTYPE_THREELAP
};

enum eNitroType {
	NITRO_NONE,
	NITRO_FULL,
	NITRO_DOUBLE,
	NITRO_INFINITE,
};
int nNitroType = NITRO_FULL;
int nUpgradeLevel = 0;
int nHandlingMode = 0;
int nGhostVisuals = 2;
bool bNoProps = false;
bool bViewReplayMode = false;
bool bShowInputsWhileDriving = false;
bool bTimeTrialsEnabled = true;
bool bPBTimeDisplayEnabled = true;
bool bCurrentSessionPBTimeDisplayEnabled = true;
bool bChloeCollectionIntegration = false;
bool bLastRaceWasTimeTrial = false;
bool bReplayIgnoreMismatches = false;
bool b3LapMode = false;

#ifdef FLATOUT_UC
bool bTimeTrialIsFOUC = true;
#else
bool bTimeTrialIsFOUC = false;
#endif

uint32_t n3LapTotalTime = 0;

void WriteLog(const std::string& str) {
#ifdef FLATOUT_UC
	static auto file = std::ofstream("FlatOutUCTimeTrialGhosts_gcp.log");
#else
	static auto file = std::ofstream("FlatOut2TimeTrialGhosts_gcp.log");
#endif

	file << str;
	file << "\n";
	file.flush();
}

struct tCarState {
	NyaMat4x4 mat;
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
		mat = *car->GetMatrix();
		vel = *car->GetVelocity();
		tvel = *car->GetAngVelocity();
		memcpy(quat, car->qQuaternion, sizeof(quat));
		nitro = car->fNitro;
		damage = car->fDamage;
		steer = car->fSteerAngle;
		gas = car->fGasPedal;
		brake = car->fBrakePedal;
		nitroHeld = car->fNitroButton;
		handbrake = car->fHandbrake;
#ifndef FLATOUT_UC
		gear = car->mGearbox.nGear;
#endif
	}

	void Apply(Player* pPlayer) {
		auto car = pPlayer->pCar;
		*car->GetMatrix() = mat;
		*car->GetVelocity() = vel;
		*car->GetAngVelocity() = tvel;
		memcpy(car->qQuaternion, quat, sizeof(quat));
		car->fNitro = nitro;
		car->fDamage = damage;
		car->fSteerAngle = steer;
		car->fGasPedal = gas;
		car->fBrakePedal = brake;
		car->fNitroButton = nitroHeld;
		car->fHandbrake = handbrake;
#ifndef FLATOUT_UC
		car->mGearbox.nGear = gear;
#endif
	}
};

struct tInputState {
	uint8_t keys[20];
};

std::vector<tCarState> aRecordingGhost;
std::vector<tInputState> aRecordingInputs;

struct tGhostSetup;
std::vector<tGhostSetup*> aGhosts;

struct tGhostSetup {
	std::vector<tCarState> aPBGhost;
	std::vector<tInputState> aPBInputs;
	uint32_t nPBTime = UINT_MAX;
	uint32_t nCurrentSessionPBTime = UINT_MAX;
	float fTextHighlightTime = 0;
	float fCurrentSessionTextHighlightTime = 0;

	tGhostSetup() {
		aGhosts.push_back(this);
	}

	bool IsValid() {
		return nPBTime != UINT_MAX;
	}

	void UpdateTextHighlight() {
		if (fTextHighlightTime > 0) fTextHighlightTime -= 0.01;
		if (fCurrentSessionTextHighlightTime > 0) fCurrentSessionTextHighlightTime -= 0.01;
	}
};
tGhostSetup RollingLapPB;
tGhostSetup StandingLapPB;
tGhostSetup ThreeLapPB;
tGhostSetup OpponentRollingLapPB;
tGhostSetup OpponentStandingLapPB;
tGhostSetup OpponentThreeLapPB;

bool bGhostLoaded = false;
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

std::string RemoveSpacesFromString(const std::string& str) {
	auto newStr = str;

	while (true) {
		auto pos = newStr.find(' ');
		if (pos == std::string::npos) break;
		newStr.erase(newStr.begin() + pos);
	}

	return newStr;
}

std::string GetGhostFilename(int car, int track, int lapType, bool isOpponentGhost, bool useLegacyNaming) {
	std::string path = "Ghosts/";
	if (isOpponentGhost) path += "Opponents/";

	if (useLegacyNaming) {
		path += "Track" + std::to_string(track) + "_Car" + std::to_string(car + 1);
	}
	else {
		path += RemoveSpacesFromString(GetTrackName(track)) + (std::string)"_" + RemoveSpacesFromString(GetCarName(car));
	}
	if (bNoProps) path += "_noprops";
	if (lapType == LAPTYPE_STANDING) path += "_lap1";
	if (lapType == LAPTYPE_THREELAP) path += "_3lap";
	switch (nNitroType) {
		case NITRO_NONE:
			path += useLegacyNaming ? "_nonitro" : "_0x";
			break;
		case NITRO_DOUBLE:
			path += useLegacyNaming ? "_2xnitro" : "_2x";
			break;
		case NITRO_INFINITE:
			path += useLegacyNaming ? "_infnitro" : "_inf";
			break;
		default:
			break;
	}
	if (nUpgradeLevel) {
		path += (useLegacyNaming ? "_upgrade" : "_up") + std::to_string(nUpgradeLevel);
	}
	if (nHandlingMode) {
		path += (useLegacyNaming ? "_handling" : "_hnd") + std::to_string(nHandlingMode);
	}
#ifdef FLATOUT_UC
	path += ".fouc";
#else
	path += ".fo2";
#endif
	path += useLegacyNaming ? "replay" : "rep";
	return path;
}

void SavePB(tGhostSetup* ghost, int car, int track, uint8_t lapType) {
	std::filesystem::create_directory("Ghosts");
	std::filesystem::create_directory("Ghosts/Opponents");

	auto fileName = GetGhostFilename(car, track, lapType, false, false);
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
	outFile.write((char*)&lapType, sizeof(lapType));
	outFile.write((char*)&bNoProps, sizeof(bNoProps));
	outFile.write((char*)&nUpgradeLevel, sizeof(nUpgradeLevel));
	outFile.write((char*)&nHandlingMode, sizeof(nHandlingMode));
	outFile.write((char*)&bTimeTrialIsFOUC, sizeof(bTimeTrialIsFOUC));
	int count = ghost->aPBGhost.size();
	outFile.write((char*)&count, sizeof(count));
	outFile.write((char*)&ghost->aPBGhost[0], sizeof(ghost->aPBGhost[0]) * count);
	outFile.write((char*)&ghost->aPBInputs[0], sizeof(ghost->aPBInputs[0]) * count);
}

void LoadPB(tGhostSetup* ghost, int car, int track, int lapType, bool isOpponentGhost) {
	// reset current session PBs if we're loading a different track/car combo
	{
		static int prevCar = -1;
		static int prevTrack = -1;
		static int prevNitro = -1;
		static int prevProps = -1;
		static int prevUpgrades = -1;
		static int prevHandling = -1;
		static int prev3Lap = -1;
		if (prev3Lap != (lapType == LAPTYPE_THREELAP) || prevCar != car || prevTrack != track || prevNitro != nNitroType || prevProps != bNoProps || prevUpgrades != nUpgradeLevel || prevHandling != nHandlingMode) {
			for (auto ghost : aGhosts) {
				ghost->nCurrentSessionPBTime = UINT_MAX;
				ghost->fCurrentSessionTextHighlightTime = 0;
			}
		}
		prev3Lap = lapType == LAPTYPE_THREELAP;
		prevCar = car;
		prevTrack = track;
		prevNitro = nNitroType;
		prevProps = bNoProps;
		prevUpgrades = nUpgradeLevel;
		prevHandling = nHandlingMode;
	}

	ghost->aPBGhost.clear();
	ghost->aPBInputs.clear();
	ghost->nPBTime = UINT_MAX;
	ghost->fTextHighlightTime = 0;

	auto fileName = GetGhostFilename(car, track, lapType, isOpponentGhost, false);
	auto inFile = std::ifstream(fileName, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) {
		auto legacyFileName = GetGhostFilename(car, track, lapType, isOpponentGhost, true);
		inFile = std::ifstream(legacyFileName, std::ios::in | std::ios::binary);
		if (!inFile.is_open()) {
			WriteLog("No ghost found for " + fileName);
			return;
		}
		inFile.close();
		WriteLog("Legacy ghost found, renaming " + legacyFileName + " to " + fileName);
		std::filesystem::rename(legacyFileName, fileName);
		return LoadPB(ghost, car, track, lapType, isOpponentGhost);
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
	uint8_t tmpfirstlap = 0;
	bool tmpnoprops = false;
	int tmpupgrade = 0;
	int tmphandling = 0;
	bool tmpfouc = false;
	int tmpnitro = NITRO_FULL;
	inFile.read((char*)&tmpcar, sizeof(tmpcar));
	inFile.read((char*)&tmptrack, sizeof(tmptrack));
	inFile.read((char*)&tmptime, sizeof(tmptime));
	if (fileVersion >= 2) {
		inFile.read((char*)&tmpnitro, sizeof(tmpnitro));
		inFile.read((char*)&tmpfirstlap, sizeof(tmpfirstlap));
		inFile.read((char*)&tmpnoprops, sizeof(tmpnoprops));
	}
	if (fileVersion >= 4) {
		inFile.read((char*)&tmpupgrade, sizeof(tmpupgrade));
		inFile.read((char*)&tmphandling, sizeof(tmphandling));
		inFile.read((char*)&tmpfouc, sizeof(tmpfouc));
	}
	if (tmpsize != sizeof(tCarState)) {
		WriteLog("Outdated ghost for " + fileName);
		return;
	}
	if (!bReplayIgnoreMismatches) {
#ifndef FLATOUT_UC
		if (tmpcar != car || tmptrack != track) {
			WriteLog("Mismatched ghost for " + fileName);
			return;
		}
#endif
		if (tmpfirstlap != lapType || tmpnoprops != bNoProps || tmpnitro != nNitroType) {
			WriteLog("Mismatched ghost for " + fileName);
			return;
		}
		if (tmpupgrade != nUpgradeLevel || tmphandling != nHandlingMode || tmpfouc != bTimeTrialIsFOUC) {
			WriteLog("Mismatched ghost for " + fileName);
			return;
		}
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
	if (b3LapMode) {
		LoadPB(&ThreeLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, LAPTYPE_THREELAP, false);
		LoadPB(&OpponentThreeLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, LAPTYPE_THREELAP, true);
	}
	else {
		LoadPB(&StandingLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, LAPTYPE_STANDING, false);
		LoadPB(&RollingLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, LAPTYPE_ROLLING, false);
		LoadPB(&OpponentStandingLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, LAPTYPE_STANDING, true);
		LoadPB(&OpponentRollingLapPB, GetPlayer(0)->nCarId, pGame->nLevelId, LAPTYPE_ROLLING, true);
	}
	bGhostLoaded = true;
}

void InvalidateGhost() {
	bGhostLoaded = false;
	fGhostTime = 0;
	fGhostRecordTotalTime = 0;
	for (auto ghost : aGhosts) {
		ghost->nPBTime = UINT_MAX;
		ghost->fTextHighlightTime = 0;
		ghost->aPBGhost.clear();
		ghost->aPBInputs.clear();
	}
	aRecordingGhost.clear();
	aRecordingInputs.clear();
}

int GetCurrentPlaybackTick(tGhostSetup* ghost) {
	double currTime = fGhostTime;
	int tick = std::floor(100 * currTime);
	if (tick > ghost->aPBGhost.size() - 1) tick = ghost->aPBGhost.size() - 1;
	return tick;
}

void RunGhost(Player* pPlayer) {
	if (!pPlayer) return;

#ifdef FLATOUT_UC
	auto eventData = tEventData(EVENT_PLAYER_RESPAWN_GHOST);
	eventData.data[3] = 1000;
	pPlayer->TriggerEvent(&eventData);
#else
	int eventProperties[] = {PLAYEREVENT_RESPAWN_GHOST, 0, 0, 0, 1000};
	pPlayer->TriggerEvent(eventProperties);
#endif

	bool isOpponent = pPlayer == GetPlayer(2);
	if (!isOpponent) fGhostTime += 0.01;

	auto ply = GetPlayerScore<PlayerScoreRace>(1);
	tGhostSetup* ghost = nullptr;
	if (isOpponent) {
		ghost = ply->nCurrentLap == 0 ? &OpponentStandingLapPB : &OpponentRollingLapPB;
	}
	else {
		ghost = ply->nCurrentLap == 0 ? &StandingLapPB : &RollingLapPB;
	}
	if (b3LapMode) ghost = isOpponent ? &OpponentThreeLapPB : &ThreeLapPB;

	if (ghost->aPBGhost.empty()) {
		if (!bViewReplayMode) pPlayer->pCar->GetMatrix()->p = {500,-25,500};
		return;
	}

	if (bViewReplayMode) SetPlayerControl(false);
	ghost->aPBGhost[GetCurrentPlaybackTick(ghost)].Apply(pPlayer);
}

tInputState RecordInputs(Player* pPlayer) {
	static tInputState inputState;
	memset(&inputState, 0, sizeof(inputState));
#ifndef FLATOUT_UC
	if (pPlayer->pController->_4[-1] == pControllerVTable) {
		for (int i = 0; i < 20; i++) {
			auto value =  GetAnalogInput(pPlayer->pController, &pPlayer->pController->aInputs[i]) * 128;
			if (value < 0) inputState.keys[i] = 0;
			else inputState.keys[i] = value;
		}
	}
	else {
		for (int i = 0; i < 20; i++) {
			inputState.keys[i] = pPlayer->pController->GetInputValue(i);
		}
	}
#endif
	return inputState;
}

void RecordGhost(Player* pPlayer) {
	if (!pPlayer) return;
#ifndef FLATOUT_UC
	if (!pPlayer->pController) return;
#endif

	fGhostRecordTotalTime += 0.01;

	tCarState state;
	state.Collect(pPlayer);
	aRecordingGhost.push_back(state);
	aRecordingInputs.push_back(RecordInputs(pPlayer));
}

void __fastcall ProcessGhostCar(Player* pPlayer) {
	if (!bTimeTrialsEnabled) return;
	if (!pPlayer) return;

	if (bViewReplayMode) SetPlayerControl(true);

	auto localPlayer = GetPlayer(0);
	auto ghostPlayer = GetPlayer(1);
	auto opponentGhostPlayer = GetPlayer(2);
	if (!localPlayer || !ghostPlayer || !opponentGhostPlayer) return;

	if (pPlayer == localPlayer) {
		for (auto ghost : aGhosts) {
			ghost->UpdateTextHighlight();
		}
	}

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
		auto localPlayerPos = localPlayer->pCar->GetMatrix()->p;
		auto ghostPlayerPos = ghostPlayer->pCar->GetMatrix()->p;
		auto opponentGhostPlayerPos = opponentGhostPlayer->pCar->GetMatrix()->p;
		SetGhostVisuals((localPlayerPos - ghostPlayerPos).length() < 8 || (localPlayerPos - opponentGhostPlayerPos).length() < 8);
	}

	auto ply = GetPlayerScore<PlayerScoreRace>(1);
	if (IsPlayerStaging(localPlayer) && ply->nCurrentLap == 0) {
		InvalidateGhost();
	}

	if (ShouldGhostRun()) {
		if (!bGhostLoaded) ResetAndLoadPBGhost();

		if (bViewReplayMode) {
			if (pPlayer == localPlayer) RunGhost(pPlayer);
			if (pPlayer == ghostPlayer || pPlayer == opponentGhostPlayer) {
				pPlayer->pCar->GetMatrix()->p = {500,-25,500};
				*pPlayer->pCar->GetVelocity() = {0,0,0};
				*pPlayer->pCar->GetAngVelocity() = {0,0,0};
				pPlayer->pCar->fGasPedal = 0;
				pPlayer->pCar->fBrakePedal = 0;
				pPlayer->pCar->fHandbrake = 1;
			}
		}
		else {
			if (pPlayer == ghostPlayer || pPlayer == opponentGhostPlayer) RunGhost(pPlayer);
			if (pPlayer == localPlayer) RecordGhost(pPlayer);
		}
	}
	else if (pPlayer == ghostPlayer || pPlayer == opponentGhostPlayer) {
		*pPlayer->pCar->GetVelocity() = {0,0,0};
		*pPlayer->pCar->GetAngVelocity() = {0,0,0};
		pPlayer->pCar->fGasPedal = 0;
		pPlayer->pCar->fBrakePedal = 0;
		pPlayer->pCar->fHandbrake = 1;
	}
}

void __fastcall OnFinishLap(uint32_t lapTime) {
	auto ply = GetPlayerScore<PlayerScoreRace>(1);
	bool isFirstLap = (ply->nCurrentLap - 1) == 0; // it's already increased by this point
	if (b3LapMode) {
		if (isFirstLap) n3LapTotalTime = lapTime;
		else n3LapTotalTime += lapTime;

		if (ply->nCurrentLap != 3) return;
	}

	uint32_t replayTime = b3LapMode ? n3LapTotalTime : lapTime;

	auto ghost = isFirstLap ? &StandingLapPB : &RollingLapPB;
	if (b3LapMode) ghost = &ThreeLapPB;

	if (!bViewReplayMode) {
		if (replayTime < ghost->nPBTime) {
			WriteLog("Saving new lap PB of " + std::to_string(replayTime) + "ms");
			ghost->aPBGhost = aRecordingGhost;
			ghost->aPBInputs = aRecordingInputs;
			ghost->nPBTime = replayTime;
			ghost->fTextHighlightTime = 5;
			SavePB(ghost, GetPlayer(0)->nCarId, pGame->nLevelId, b3LapMode ? LAPTYPE_THREELAP : isFirstLap);
		}
		if (replayTime < ghost->nCurrentSessionPBTime) {
			ghost->nCurrentSessionPBTime = replayTime;
			ghost->fCurrentSessionTextHighlightTime = 5;
		}
	}
	isFirstLap = false;
	aRecordingGhost.clear();
	aRecordingInputs.clear();
	fGhostTime = 0;
	fGhostRecordTotalTime = 0;
}

const wchar_t* GetAIName() {
	static bool bType = false;
	bType = !bType;
	return bType ? L"PB GHOST" : L"OPPONENT GHOST";
}

auto gInputRGBBackground = NyaDrawing::CNyaRGBA32(215,215,215,255);
auto gInputRGBHighlight = NyaDrawing::CNyaRGBA32(0,255,0,255);
float fInputBaseXPosition = 0.2;
float fInputBaseYPosition = 0.85;

void DrawInputTriangle(float posX, float posY, float sizeX, float sizeY, float inputValue, bool invertValue) {
	float minX = std::min(posX - sizeX, posX + sizeX);
	float maxX = std::max(posX - sizeX, posX + sizeX);

	DrawTriangle(posX, posY - sizeY, posX - sizeX, std::lerp(posY - sizeY, posY + sizeY, 0.5), posX,
				 posY + sizeY, invertValue ? gInputRGBBackground : gInputRGBHighlight);

	DrawTriangle(posX, posY - sizeY, posX - sizeX, std::lerp(posY - sizeY, posY + sizeY, 0.5), posX,
				 posY + sizeY, invertValue ? gInputRGBHighlight : gInputRGBBackground, std::lerp(minX, maxX, inputValue), 0, 1, 1);
}

void DrawInputTriangleY(float posX, float posY, float sizeX, float sizeY, float inputValue, bool invertValue) {
	float minY = std::min(posY - sizeY, posY + sizeY);
	float maxY = std::max(posY - sizeY, posY + sizeY);

	DrawTriangle(std::lerp(posX - sizeX, posX + sizeX, 0.5), posY - sizeY, posX - sizeX, posY + sizeY, posX + sizeX,
				 posY + sizeY, invertValue ? gInputRGBBackground : gInputRGBHighlight);

	DrawTriangle(std::lerp(posX - sizeX, posX + sizeX, 0.5), posY - sizeY, posX - sizeX, posY + sizeY, posX + sizeX,
				 posY + sizeY, invertValue ? gInputRGBHighlight : gInputRGBBackground, 0, std::lerp(minY, maxY + 0.001, inputValue), 1, 1);
}

void DrawInputRectangle(float posX, float posY, float scaleX, float scaleY, float inputValue) {
	DrawRectangle(posX - scaleX, posX + scaleX, posY - scaleY, posY + scaleY, gInputRGBBackground);
	DrawRectangle(posX - scaleX, posX + scaleX, std::lerp(posY + scaleY, posY - scaleY, inputValue), posY + scaleY, gInputRGBHighlight);
}

void DisplayInputs(tInputState* inputs) {
#ifndef FLATOUT_UC
	DrawInputTriangle((fInputBaseXPosition - 0.005) * GetAspectRatioInv(), fInputBaseYPosition, 0.08 * GetAspectRatioInv(), 0.07, 1 - (inputs->keys[INPUT_STEER_LEFT] / 128.0), true);
	DrawInputTriangle((fInputBaseXPosition + 0.08) * GetAspectRatioInv(), fInputBaseYPosition, -0.08 * GetAspectRatioInv(), 0.07, inputs->keys[INPUT_STEER_RIGHT] / 128.0, false);
	DrawInputTriangleY((fInputBaseXPosition + 0.0375) * GetAspectRatioInv(), fInputBaseYPosition - 0.05, 0.035 * GetAspectRatioInv(), 0.045, 1 - (inputs->keys[INPUT_ACCELERATE] / 128.0), true);
	DrawInputTriangleY((fInputBaseXPosition + 0.0375) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.035 * GetAspectRatioInv(), -0.045, inputs->keys[INPUT_BRAKE] / 128.0, false);

	DrawInputTriangleY((fInputBaseXPosition + 0.225) * GetAspectRatioInv(), fInputBaseYPosition - 0.04, 0.035 * GetAspectRatioInv(), 0.035, 1 - (inputs->keys[INPUT_GEAR_UP] / 128.0), true);
	DrawInputTriangleY((fInputBaseXPosition + 0.225) * GetAspectRatioInv(), fInputBaseYPosition + 0.04, 0.035 * GetAspectRatioInv(), -0.035, inputs->keys[INPUT_GEAR_DOWN] / 128.0, false);

	DrawInputRectangle((fInputBaseXPosition + 0.325) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->keys[INPUT_NITRO] / 128.0);
	DrawInputRectangle((fInputBaseXPosition + 0.425) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->keys[INPUT_HANDBRAKE] / 128.0);
	DrawInputRectangle((fInputBaseXPosition + 0.525) * GetAspectRatioInv(), fInputBaseYPosition + 0.05, 0.03 * GetAspectRatioInv(), 0.03, inputs->keys[INPUT_RESET] / 128.0);
#endif
}

void DrawTimeText(tNyaStringData& data, const std::string& name, uint32_t pbTime, bool isTextHighlighted) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string bestTimeString = name;
	if (pbTime == UINT_MAX) {
		bestTimeString += "N/A";
	}
	else {
		bestTimeString += GetTimeFromMilliseconds(pbTime);
		bestTimeString.pop_back(); // remove trailing 0, the game has a tickrate of 100fps
	}
	if (isTextHighlighted) data.SetColor(0, 255, 0, 255);
	else data.SetColor(255, 255, 255, 255);
	DrawStringFO2(data, converter.from_bytes(bestTimeString).c_str(), "FontLarge");
	data.y += 0.04;
}

void HookLoop() {
	if (pLoadingScreen) {
		CommonMain();
		return;
	}

	if (auto player = GetPlayer(0)) {
		auto ply = GetPlayerScore<PlayerScoreRace>(1);

#ifndef FLATOUT_UC
		if (bViewReplayMode) {
			auto ghost = ply->nCurrentLap == 0 ? &StandingLapPB : &RollingLapPB;
			if (b3LapMode) ghost = &ThreeLapPB;
			if (!ghost->aPBInputs.empty()) {
				DisplayInputs(&ghost->aPBInputs[GetCurrentPlaybackTick(ghost)]);
			}
		}
		else if (bShowInputsWhileDriving) {
			auto inputs = RecordInputs(player);
			DisplayInputs(&inputs);
		}
#endif

		if (bTimeTrialsEnabled && (bPBTimeDisplayEnabled || bCurrentSessionPBTimeDisplayEnabled) && !IsPlayerStaging(player)) {
			tNyaStringData data;
			//data.x = 0.5;
			//data.y = 0.21;
			//data.XCenterAlign = true;
			data.x = 1 - (0.05 * GetAspectRatioInv());
			data.y = 0.17;
#ifdef FLATOUT_UC
			data.y += 0.06;
#endif
			data.size = 0.04;
			data.XRightAlign = true;
			if (b3LapMode) {
				if (bPBTimeDisplayEnabled) {
					DrawTimeText(data, "Best Time: ", ThreeLapPB.nPBTime, ThreeLapPB.fTextHighlightTime > 0);
				}
				if (bCurrentSessionPBTimeDisplayEnabled) {
					DrawTimeText(data, "Best Time (Session): ", ThreeLapPB.nCurrentSessionPBTime,
								 ThreeLapPB.fCurrentSessionTextHighlightTime > 0);
				}
				if (bPBTimeDisplayEnabled && OpponentThreeLapPB.IsValid()) {
					DrawTimeText(data, "Opponent's Best Time: ", OpponentThreeLapPB.nPBTime, OpponentThreeLapPB.fTextHighlightTime > 0);
				}
			}
			else {
				if (bPBTimeDisplayEnabled) {
					DrawTimeText(data, "Rolling PB: ", RollingLapPB.nPBTime, RollingLapPB.fTextHighlightTime > 0);
					DrawTimeText(data, "Standing PB: ", StandingLapPB.nPBTime, StandingLapPB.fTextHighlightTime > 0);
				}
				if (bCurrentSessionPBTimeDisplayEnabled) {
					DrawTimeText(data, "Rolling PB (Session): ", RollingLapPB.nCurrentSessionPBTime,
								 RollingLapPB.fCurrentSessionTextHighlightTime > 0);
					DrawTimeText(data, "Standing PB (Session): ", StandingLapPB.nCurrentSessionPBTime,
								 StandingLapPB.fCurrentSessionTextHighlightTime > 0);
				}
				if (bPBTimeDisplayEnabled) {
					if (OpponentRollingLapPB.IsValid()) {
						DrawTimeText(data, "Opponent Rolling PB: ", OpponentRollingLapPB.nPBTime, OpponentRollingLapPB.fTextHighlightTime > 0);
					}
					if (OpponentStandingLapPB.IsValid()) {
						DrawTimeText(data, "Opponent Standing PB: ", OpponentStandingLapPB.nPBTime, OpponentStandingLapPB.fTextHighlightTime > 0);
					}
				}
			}
		}
	}

#ifdef FLATOUT_UC
	// uninit time trials once we're back in the menu
	if (bChloeCollectionIntegration && pGame) {
		static auto nLastGameState = pGame->nGameState;
		auto currentGameState = pGame->nGameState;
		if (currentGameState != nLastGameState) {
			if (currentGameState == GAME_STATE_MENU && bTimeTrialsEnabled) {
				UninitTimeTrials();
				bTimeTrialsEnabled = false;
			}
			if (currentGameState == GAME_STATE_RACE && !bTimeTrialsEnabled) {
				bLastRaceWasTimeTrial = false;
			}
			nLastGameState = currentGameState;
		}
	}
#endif

	CommonMain();
}

void InitAndReadConfigFile() {
	// at 30fps this should be a few minutes?
	aRecordingGhost.reserve(10000);
	aRecordingInputs.reserve(10000);

#ifdef FLATOUT_UC
	auto config = toml::parse_file("FlatOutUCTimeTrialGhosts_gcp.toml");
#else
	auto config = toml::parse_file("FlatOut2TimeTrialGhosts_gcp.toml");
#endif
	bTimeTrialsEnabled = config["main"]["enabled"].value_or(true);
	nGhostVisuals = config["main"]["ghost_visuals"].value_or(2);
	bNoProps = config["main"]["disable_props"].value_or(false);
	nNitroType = config["main"]["nitro_option"].value_or(NITRO_FULL);
	bPBTimeDisplayEnabled = config["main"]["show_best_times"].value_or(true);
	bCurrentSessionPBTimeDisplayEnabled = config["main"]["show_best_times_in_session"].value_or(true);
	bReplayIgnoreMismatches = config["main"]["load_mismatched_replays"].value_or(false);
	b3LapMode = config["main"]["three_lap_mode"].value_or(false);
	bViewReplayMode = config["extras"]["view_replays"].value_or(false);
	bShowInputsWhileDriving = config["extras"]["always_show_input_display"].value_or(false);
	gInputRGBHighlight.r = config["input_display"]["highlight_r"].value_or(0);
	gInputRGBHighlight.g = config["input_display"]["highlight_g"].value_or(255);
	gInputRGBHighlight.b = config["input_display"]["highlight_b"].value_or(0);
	gInputRGBBackground.r = config["input_display"]["background_r"].value_or(200);
	gInputRGBBackground.g = config["input_display"]["background_g"].value_or(200);
	gInputRGBBackground.b = config["input_display"]["background_b"].value_or(200);
	fInputBaseXPosition = config["input_display"]["pos_x"].value_or(0.2);
	fInputBaseYPosition = config["input_display"]["pos_y"].value_or(0.85);
}