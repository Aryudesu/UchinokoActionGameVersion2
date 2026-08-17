#pragma once
#include "Singleton.h"
#include "Scene.h"

class SceneManager{
private:
	Scene * ActiveScene;
	int NextScene = 0;
public:
	SceneManager();

	void update();
	void draw();

	void SceneChange();
};