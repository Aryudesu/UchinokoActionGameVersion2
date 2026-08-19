#include "SceneManager.h"
#include "Menu.h"
#include "StageSelect.h"
#include "SceneChanger.h"
#include "MusicRoom.h"
#include "function.h"

SceneManager::SceneManager() {
	ActiveScene = std::make_unique<Menu>();	//初期はメニュー画面
	NextScene = STEADY;			//何も変化なし
}

void SceneManager::update() {
	SceneChange();
	//シーンに変化があれば
	if (NextScene != STEADY) {
		//シーンを更新
		switch (NextScene) {
			case MENU:
				ActiveScene = std::make_unique<Menu>();
			break;
			case GAME:
				ActiveScene = std::make_unique<StageSelect>();
				break;
			case MUSIC:
				ActiveScene = std::make_unique<MusicRoom>();
				//TODO
				break;
		}
		NextScene = STEADY;
	}
	ActiveScene->update();
}

void SceneManager::draw() {
	ActiveScene->draw();
}

void SceneManager::SceneChange() {
	NextScene = SceneChanger::GetInstance().GetSceneNum();
	SceneChanger::GetInstance().ResetScene();
}
