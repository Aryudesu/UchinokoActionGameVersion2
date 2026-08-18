#include "StageSelect.h"
#include "function.h"
#include "Action.h"
#include "WorldMap.h"
#include "LevelChanger.h"
#include "GameData.h"
#include "Souko.h"
#include <memory>
#include <sstream>
#include <fstream>
#include <iterator>

StageSelect::StageSelect() {
	Games = new WorldMap("dat/wmap/wmblock.ary","dat/wmap/wmlay.ary");
	NextLevel = NONES;
}


void StageSelect::LoadData(int StageNum) {
	INIDat* STDat = LoadStageData(StageNum);
	int GameMode = std::stoi(STDat->GetData("StageData","GameMode")[0]);
	switch (GameMode) {
	case -1:
		break;
	case 0:
		Games = new Action(StageNum,STDat);
		delete STDat;
		break;
	case 1:
		Games = new Souko(StageNum, STDat);
		delete STDat;
		break;
	default:
		exit(0);
		break;
	}
}

void StageSelect::update() {
	LevelUpdate();
	if (NextLevel != NONES) {
		delete Games;
		if (NextLevel == WMAPS) {
			Games = new WorldMap("dat/wmap/wmblock.ary", "dat/wmap/wmlay.ary");
		} else {
			LoadData(NextLevel);
		}
		NextLevel = NONES;
	}
	Games->update();
}


void StageSelect::draw() {
	Games->draw();
}

void StageSelect::LevelUpdate() {
	NextLevel = LevelChanger::GetInstance().GetLevelNum();
	LevelChanger::GetInstance().ResetLevel();
}
