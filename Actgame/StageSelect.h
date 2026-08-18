#pragma once
#include "Scene.h"
#include "GameCollection.h"

#define WMAP -1

class StageSelect :public Scene {
	GameCollection *Games;
	GameCollection *WM;
	int NextLevel;
public:
	StageSelect();
	void LoadData(int StageNum);
	void update();
	void draw();
	void LevelUpdate();
};