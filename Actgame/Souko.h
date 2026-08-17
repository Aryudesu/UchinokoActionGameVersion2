#pragma once
#include "Player.h"
#include "GameCollection.h"
#include "LoadIni.h"
#include <vector>
#include <string>

class Souko : public GameCollection {
private:
	std::vector<std::vector<int>> Map;
	std::vector<std::vector<int>> ResetMap;
	int StageDetailNum;
	int BeatLevelTime;
	int StageNumber;
	int Step,StepMax;
	void LoadBGM(INIDat* SDList);
	int sx, sy;
	int direx, direy;
	int movetime;
	int animetime;
	bool Beat;
	int BeatTime;
	bool move;
	int Bright;
	int BrightF;
	bool ResetF;
	void LoadImg();
	void LoadImg(INIDat* SDList);
	void Reset();
	bool CheckBeat();
	void SCharaMove();
	void LoadSound();
public:
	Souko();
	Souko(int StageNum, INIDat* STDat);
	~Souko();
	void LoadMapData(int StageNum, INIDat* SDList);
	int GetBeatLevel();
	bool update();
	void draw();
};