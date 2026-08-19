#pragma once
#include "Singleton.h"
#include "Scene.h"
#include <memory>

class SceneManager{
private:
	std::unique_ptr<Scene> ActiveScene;
	int NextScene = 0;
public:
	SceneManager();

	void update();
	void draw();

	void SceneChange();
};
