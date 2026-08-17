#include "DxLib.h"
#include "Menu.h"
#include "Conf.h"
#include "SceneChanger.h"
#include "GameData.h"
#include "InputKey.h"
#include "function.h"

Menu::Menu() {
	//最初にカーソルがある箇所
	Select = 0;
}


void Menu::update() {
	//下押した時
	if (ReturnKey(KEY_INPUT_DOWN) % 10 == 1) {
		Select = (Select + 1) % SelectStr.size();
	}//上押した時
	else if(ReturnKey(KEY_INPUT_UP) % 10 == 1){
		Select = (Select + SelectStr.size() - 1) % SelectStr.size();
	}
	//決定ボタン押した時
	if (ReturnKey(KEY_INPUT_Z) > 0) {
		switch(Select){
			case SELECT_GAME:
				GameData::GetInstance().DataLoad();
				SceneChanger::GetInstance().Change(GAME);
			break;
			case MUSIC_ROOM:
				SceneChanger::GetInstance().Change(MUSIC);
				break;
			default:
				exit(0);
			break;
		}
	}
}

void Menu::draw() {
	//メニュー描画
	for (int i = 0; i < SelectStr.size(); i++)DrawString(100, WINDOWY / 2 + i*Space, SelectStr[i].c_str() , GetColor(255, 255, 255));
	DrawString(100 - Space, WINDOWY / 2 + Select * Space, "■", GetColor(255, 255, 255));
}