#include "SceneManager.h"
#include "Menu.h"
#include "StageSelect.h"
#include "SceneChanger.h"
#include "MusicRoom.h"
#include "function.h"
#include "SlopeSandboxScene.h"
#include "GimmickSandboxScene.h"

SceneManager::SceneManager() {
	ActiveScene = new Menu();	//初期はメニュー画面
	NextScene = STEADY;			//何も変化なし
}

SceneManager::~SceneManager() {
	delete ActiveScene;
}

void SceneManager::update() {
	SceneChange();
	//シーンに変化があれば
	if (NextScene != STEADY) {
		delete ActiveScene;
		//シーンを更新
		switch (NextScene) {
			case MENU:
				ActiveScene = new Menu();
			break;
			case GAME:
				ActiveScene = new StageSelect();
				break;
			case MUSIC:
				ActiveScene = new MusicRoom();
				//TODO
				break;
			case SLOPE_SANDBOX:
				ActiveScene = new SlopeSandboxScene();
				break;
			case GIMMICK_SANDBOX:
				ActiveScene = new GimmickSandboxScene();
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
