#pragma once
#include "Singleton.h"

#define NONES 0
#define WMAPS 1
#define GAMES 2

class LevelChanger : public Singleton<LevelChanger> {
private:
	int NowScene = NONES;
public:
	void Change(int n);
	int GetLevelNum();
	void ResetLevel();
};