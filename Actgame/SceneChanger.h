#pragma once
#include "Singleton.h"


#define STEADY 0
#define MENU   1
#define GAME   2
#define MUSIC  3
#define SLOPE_SANDBOX 4

class SceneChanger :public Singleton<SceneChanger>{
private:
	int NowScene;
public:
	void Change(int n);
	int GetSceneNum();
	void ResetScene();
};
