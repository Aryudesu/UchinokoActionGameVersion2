#pragma once
#include "Scene.h"
#include <vector>
#include <string>

#define SELECT_GAME 0
#define MUSIC_ROOM  1
#define SLOPE_TEST  2
#define GIMMICK_TEST 3

class Menu :public Scene{
protected:
	int Select;
	int Space = 30;
	std::vector<std::string> SelectStr = { "開始","サウンドトラック","Slope test","Gimmick test","終了" };
public:
	Menu();
	void update();
	void draw();
};
