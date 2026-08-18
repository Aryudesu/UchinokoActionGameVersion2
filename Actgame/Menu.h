#pragma once
#include "Scene.h"
#include <vector>
#include <string>

#define SELECT_GAME 0
#define MUSIC_ROOM  1

class Menu :public Scene{
protected:
	int Select;
	int Space = 30;
	std::vector<std::string> SelectStr = { "開始","サウンドトラック","終了" };
public:
	Menu();
	void update();
	void draw();
};