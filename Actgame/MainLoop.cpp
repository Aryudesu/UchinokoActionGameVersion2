#include "MainLoop.h"
#include "Action.h"
#include "InputKey.h"
#include "SoundManager.h"

GameBody::GameBody() {
	SceneMng = new SceneManager();
}

bool GameBody::loop() {
	//メインの動きを書いていく
	//キーボード状態更新
	KeyUpdate();
	SceneMng->update();
	SceneMng->draw();
	SoundManager::GetInstance().ConfBGMVolume();
	SoundManager::GetInstance().ConfVolume();
	SoundManager::GetInstance().Draw();
	return true;
}