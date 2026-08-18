#pragma once
#include "Scene.h"
#include "GameCollection.h"
#include <memory>

#define WMAP -1

class StageSelect :public Scene {
	std::unique_ptr<GameCollection> Games;
	int NextLevel;
public:
	StageSelect();
	void LoadData(int StageNum);
	void update();
	void draw();
	void LevelUpdate();
};
