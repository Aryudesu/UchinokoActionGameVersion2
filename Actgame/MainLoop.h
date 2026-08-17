#pragma once
#include "SceneManager.h"

class GameBody {
private:
	SceneManager *SceneMng;

public:
	GameBody();
	~GameBody() = default;
	bool loop();
};