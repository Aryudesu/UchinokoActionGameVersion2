#pragma once
#include "Player.h"
#include "GameCollection.h"
#include "LoadIni.h"
#include <vector>
#include <string>

class Action : public GameCollection{
private:
	int DebugPrevNum = 0,DebugAfterNum = 0;

	int BeatLevel = 0;
	int BeatLevelTime = 0;

	bool StartF;
	bool FadeInF;
	bool FadeOutF;
	bool FadeF;
	int FadeTime;
	const int FadeTimeMax = 256;


	Map *M;
	int ScrollMode;
	int Appear;
	int StageNumber;
	int StageDetailNum;
	std::vector<int> StageMove;

	int PrevDetailNum;
	bool Moving;

	void MovingUpdate();
	void NormalUpdate();
	void DeadUpdate();

	void LoadImg(INIDat* SDList);
	void LoadImg();
	void LoadBGM(INIDat* SDList);
	void LoadSound(INIDat* SDList);
	void LoadSound();
	void LoadMapData(int StageNum, INIDat* SDList);

	void Move(int Num);

	void DeleteAll();

	int GetMove();
	int GetMoveTime();

	void FadeIn();
	void FadeOut();

public:
	Action();
	Action(int StageNum, INIDat* STDat);
	~Action();
	int GetBeatLevel();
	bool update();
	void draw();
};