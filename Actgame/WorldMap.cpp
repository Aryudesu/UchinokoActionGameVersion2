#include "WorldMap.h"
#include "function.h"
#include "ImageManager.h"
#include "Conf.h"
#include "InputKey.h"
#include "LevelChanger.h"
#include "GameData.h"
#include "SoundManager.h"
#include "SceneChanger.h"
#include "PlayerManager.h"

WorldMap::WorldMap(std::string MapDat, std::string ImgDat) {
	GameOverTime = 0;
	GameOverF = false;
	SoundManager::GetInstance().SetSE(DECIDE, "dat/SE/stage.wav");
	PAnime = 0;
	PDire = 0;
	Data = LoadArray(MapDat);
	ImgData = LoadArray(ImgDat);
	SCLU.x = SCLU.y = 0;
	pos.x = pos.y = 0;
	for (int i = 0; i < Data.size(); i++) {
		for (int j = 0; j < Data[i].size(); j++) {
			if (Data[i][j] == 999) {
				Data[i][j] = -999;
				pos.x = j * 32;
				pos.y = i * 32;
			}
			if (Data[i][j] < 0) {
				if (GameData::GetInstance().GetBeatLevel(-Data[i][j]) == 1) {
					Data[i][j] = 1;
				}
			}
		}
	}
	if (GameData::GetInstance().GetWMPos().x != -1 && GameData::GetInstance().GetWMPos().y != -1) {
		pos.x = GameData::GetInstance().GetWMPos().x;
		pos.y = GameData::GetInstance().GetWMPos().y;
	}
	ImageManager::GetInstance().SetTrans(TRANSR, TRANSG, TRANSB);					//透過色指定
	ImageManager::GetInstance().LoadImg(WMAPTILE,9,25,"dat/img/WMLayer.bmp");			//画像読み込み
	ImageManager::GetInstance().LoadImg(WMAPPOINT, 10, 1, "dat/img/WMPoint.bmp");	//画像読み込み
	ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 24, "dat/img/maincharactor.bmp");	//画像読み込み
}

WorldMap::WorldMap() {}

WorldMap::~WorldMap() {
	ImageManager::GetInstance().DeleteAll();
}

bool WorldMap::IsBlock(int Num) {
	return (Data[(pos.y + 16) / 32][(pos.x + 16) / 32] <= 0 && Data[(pos.y + 16) / 32][(pos.x + 16) / 32] > -500);
}

void WorldMap::PlayerUpdate() {
	float speed = 3.;
	int Bl = Data[(pos.y + 16) / 32][(pos.x + 16) / 32];
	PImgBase = PDire;
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) {
		PDire = 0;
		pos.x += speed;
		PImgBase = 2;
		if (IsBlock(Bl))pos.x -= speed;
	}
	if (ReturnKey(KEY_INPUT_LEFT) != 0) {
		PDire = 1;
		pos.x -= speed;
		PImgBase = 3;
		if (IsBlock(Bl))pos.x += speed;
	}

	if (ReturnKey(KEY_INPUT_DOWN) != 0) {
		pos.y += speed;
		PImgBase = 11;
		if (IsBlock(Bl))pos.y -= speed;
	}
	if (ReturnKey(KEY_INPUT_UP) != 0) {
		pos.y -= speed;
		PImgBase = 12;
		if (IsBlock(Bl))pos.y += speed;
	}
	SCLU.x = (int)(((pos.x + MAPSIZEX / 2) / MAPSIZEX) / SCREENX);
	SCLU.y = (int)(((pos.y + MAPSIZEY / 2) / MAPSIZEY) / SCREENY);
	SCLU.x = SCLU.x * MAPSIZEX * SCREENX;
	SCLU.y = SCLU.y * MAPSIZEY * SCREENY;
	if (ReturnKey(KEY_INPUT_Z) == 1) {
		if (Data[(pos.y + 16) / 32][(pos.x + 16) / 32] >= 5) {
			KaiwaInit(Data[(pos.y + 16) / 32][(pos.x + 16) / 32]);
			SoundManager::GetInstance().PlaySE(DECIDE);
			Kaiwa_Z = true;
		}
	}
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
	}
	//自動会話イベント
	if (Data[(pos.y + 16) / 32][(pos.x + 16) / 32] <= -500) {
		int tmp = Data[(pos.y + 16) / 32][(pos.x + 16) / 32];
		if (KaiwaExist(-tmp)) {
			if (GameData::GetInstance().GetBeatLevel(-tmp) == 0) {
				KaiwaInit(-tmp);
				Kaiwa_Z = true;
				Data[(pos.y + 16) / 32][(pos.x + 16) / 32] = 1;
				GameData::GetInstance().SetBeatLevel(-tmp);
				return;
			}
		}
		else {
			GameData::GetInstance().SetBeatLevel(-tmp);
			Data[(pos.y + 16) / 32][(pos.x + 16) / 32] = 1;
		}
	}
	PAnime++;
	if (PAnime >= 4*8)PAnime = 0;
	GameData::GetInstance().SetWMPos(pos.x,pos.y);
}

bool WorldMap::KaiwaExist(int Num) {
	std::string FileName = "dat/wmap/event/" + std::to_string(Num) + ".ary";
	int fp = FileRead_open(FileName.c_str());
	FileRead_close(fp);
	return (fp!=0);
}

std::vector<std::vector<std::string>> WorldMap::KaiwaLoad(int Num){
	std::string FileName = "dat/wmap/event/" + std::to_string(Num) + ".ary";
	return LoadStrArray(FileName);
}

void WorldMap::KaiwaInit(int Num) {
	KaiwaNum = Num;
	KaiwaLine = 0;
	Kaiwa_Z = false;
	if (KaiwaExist(Num)) {
		KaiwaDat = KaiwaLoad(Num);
		Kaiwa = true;
	} else {
		Kaiwa = false;
		LevelChanger::GetInstance().Change(Num);
	}
}

void WorldMap::KaiwaEv() {
	if (KaiwaDat[KaiwaLine][0] == "Save") {
		GameData::GetInstance().DataSave();
		KaiwaLine++;
		if (KaiwaDat.size() == KaiwaLine) {
			Kaiwa = false;
			return;
		}
	}
	if (ReturnKey(KEY_INPUT_Z) == 1 && !Kaiwa_Z) {
		KaiwaLine++;
		Kaiwa_Z = true;
		SoundManager::GetInstance().PlaySE(DECIDE);
		if (KaiwaDat.size() == KaiwaLine) {
			Kaiwa = false;
			return;
		}
		if (KaiwaDat[KaiwaLine][0] == "Start") {
			Kaiwa = false;
			LevelChanger::GetInstance().Change(KaiwaNum);
		}
	} else {
		Kaiwa_Z = false;
	}
}

void WorldMap::KaiwaDraw(int Num) {
	//会話ボックス
	DrawBox(OBJSIZEX / 2, WINDOWY / 2 + OBJSIZEX / 2, WINDOWX - OBJSIZEX / 2, WINDOWY - OBJSIZEX / 2, GetColor(0, 0, 0), TRUE);
	DrawBox(OBJSIZEX / 2 + 1, WINDOWY / 2 + OBJSIZEX / 2 + 1, WINDOWX - OBJSIZEX / 2 - 1, WINDOWY - OBJSIZEX / 2 - 1, GetColor(255, 255, 255), TRUE);
	DrawBox(OBJSIZEX / 2 + 4, WINDOWY / 2 + OBJSIZEX / 2 + 4, WINDOWX - OBJSIZEX / 2 - 4, WINDOWY - OBJSIZEX / 2 - 4, GetColor(0, 0, 0), TRUE);
	for (int i = 0; i < KaiwaDat[Num].size() - 1; i++) {
		DrawString(4 * OBJSIZEX + OBJSIZEX/2, WINDOWY / 2 + 20*(i+2), KaiwaDat[Num][i+1].c_str(), GetColor(255, 255, 255));
	}
	std::string FaceName = "dat/img/face/" + KaiwaDat[Num][0] + ".bmp";
	LoadGraphScreen(OBJSIZEX, WINDOWY / 2 + OBJSIZEY , FaceName.c_str(), TRUE);
}

bool WorldMap::update() {
	if (!Kaiwa) {
		if (GameData::GetInstance().GetZanki() <= 0)GameOverF = true;
		if (!GameOverF)PlayerUpdate();
		else GameOver();
	} else {
		KaiwaEv();
	}
	return true;
}

void WorldMap::draw(){
	for (int i = 0; i < ImgData.size(); i++) {
		for (int j = 0; j < ImgData[i].size(); j++) {
			ImageManager::GetInstance().DrawImg(j * 32 - SCLU.x, i * 32 - SCLU.y, WMAPTILE, ImgData[i][j], FALSE);
			if (Data[i][j] >= 5 && Data[i][j] < 500) {
				if (GameData::GetInstance().GetBeatLevel(Data[i][j]) != 1)ImageManager::GetInstance().DrawImg(j * 32 - SCLU.x, i * 32 - SCLU.y, WMAPPOINT, 0, TRUE);
				if (GameData::GetInstance().GetBeatLevel(Data[i][j]) == 1)ImageManager::GetInstance().DrawImg(j * 32 - SCLU.x, i * 32 - SCLU.y, WMAPPOINT, 1, TRUE);
			}
			if (Data[i][j] <  0 && Data[i][j] > -500)ImageManager::GetInstance().DrawImg(j * 32 - SCLU.x, i * 32 - SCLU.y, WMAPPOINT, 7, TRUE);
		}
	}
	ImageManager::GetInstance().DrawImg(pos.x - SCLU.x, pos.y - SCLU.y - 16, MAIN_CHARA, PImgBase * 4 + PAnime/8 , TRUE);
	if(Data[(pos.y + 16) / 32][(pos.x + 16) / 32] >= 5)ImageManager::GetInstance().DrawImg(pos.x - SCLU.x, pos.y - SCLU.y - 16, MAIN_CHARA, 4*22 + 1, TRUE);
	if (Kaiwa)KaiwaDraw(KaiwaLine);
	if(GameOverF)LoadGraphScreen(0, 0, "dat/img/gameover.bmp", FALSE);
}

void WorldMap::GameOver() {
	GameOverTime++;
	if (GameOverTime >= 5 * 60) {
		GameOverF = false;
		GameOverTime = 0;
		PlayerManager::GetInstance().SetHP(3);
		GameData::GetInstance().SetZanki(5);
		GameData::GetInstance().DataSave();
	}
}