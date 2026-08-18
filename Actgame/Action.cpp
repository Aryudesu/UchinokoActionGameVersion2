#include "Action.h"
#include "Dxlib.h"
#include "Conf.h"
#include "Object.h"
#include "Map.h"
#include "SoundManager.h"
#include "ImageManager.h"
#include "ObjectManager.h"
#include "PlayerManager.h"
#include "function.h"
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>
#include <iterator>
#include <map>
#include "InputKey.h"
#include "LevelChanger.h"
#include "GameData.h"
#include "LoadIni.h"

void Action:: LoadImg(INIDat *SDList) {
	std::string Path = "dat/img/";
	ImageManager::GetInstance().SetTrans(TRANSR, TRANSG, TRANSB);					//透過色指定
	if (SDList->CheckElem("ImgDat","MainCharactor"))ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 22, Path + SDList->GetData("ImgDat", "MainCharactor")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 22, Path + "maincharactor.bmp");	//画像読み込み

	if (SDList->CheckElem("ImgDat", "Item"))ImageManager::GetInstance().LoadImg(ITEM, 4, 4, Path + SDList->GetData("ImgDat", "Item")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(ITEM, 4, 4, Path + "item.bmp");			//画像読み込み

	if (SDList->CheckElem("ImgDat", "Enemy"))ImageManager::GetInstance().LoadImg(ENEMY, 12, 7, Path + SDList->GetData("ImgDat", "Enemy")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(ENEMY, 12, 7, Path + "enemy.bmp");		//画像読み込み

	if (SDList->CheckElem("ImgDat", "Effect"))ImageManager::GetInstance().LoadImg(EFFECT, 10, 22, Path + SDList->GetData("ImgDat", "Effect")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(EFFECT, 10, 22, Path + "effect.bmp");		//画像読み込み

	if (SDList->CheckElem("ImgDat", "Block"))ImageManager::GetInstance().LoadImg(MAP1, 10, 40, Path + SDList->GetData("ImgDat", "Block")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(MAP1, 10, 40, Path + "block.bmp");			//画像読み込み

	if (SDList->CheckElem("ImgDat", "MapObject"))ImageManager::GetInstance().LoadImg(MAPOBJ, 10, 22, Path + SDList->GetData("ImgDat", "MapObject")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(MAPOBJ, 10, 22, Path + "mapobj.bmp");		//画像読み込み

	if (SDList->CheckElem("ImgDat", "Object"))ImageManager::GetInstance().LoadImg(OBJECT, 10, 12, Path + SDList->GetData("ImgDat", "Object")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(OBJECT, 10, 12, Path + "object.bmp");		//画像読み込み

	if (SDList->CheckElem("ImgDat", "Background"))ImageManager::GetInstance().LoadImg(HAIKEI,Path + SDList->GetData("ImgDat", "Background")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(HAIKEI, Path + "haikei.bmp");				//画像読み込み

	if (SDList->CheckElem("ImgDat", "Background2"))ImageManager::GetInstance().LoadImg(HAIKEI2, Path + SDList->GetData("ImgDat", "Background2")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(HAIKEI2, Path + "haikei1_2.bmp");			//画像読み込み

	if (SDList->CheckElem("ImgDat", "Score"))ImageManager::GetInstance().LoadImg(SCORE, 16, 16,10,2, Path + SDList->GetData("ImgDat", "Score")[0]);	//画像読み込み
	else ImageManager::GetInstance().LoadImg(SCORE, 16, 16, 10, 2, Path + "score.bmp");			//画像読み込み
}

void Action::LoadImg() {
	std::string Path = "dat/img/";
	ImageManager::GetInstance().SetTrans(TRANSR, TRANSG, TRANSB);					//透過色指定
	ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 22, Path + "maincharactor.bmp");	//画像読み込み
	ImageManager::GetInstance().LoadImg(ITEM, 4, 4, Path + "item.bmp");			//画像読み込み
	ImageManager::GetInstance().LoadImg(ENEMY, 12, 7, Path + "enemy.bmp");		//画像読み込み
	ImageManager::GetInstance().LoadImg(EFFECT, 10, 22, Path + "effect.bmp");		//画像読み込み
	ImageManager::GetInstance().LoadImg(MAP1, 10, 40, Path + "block.bmp");			//画像読み込み
	ImageManager::GetInstance().LoadImg(MAPOBJ, 10, 22, Path + "mapobj.bmp");		//画像読み込み
	ImageManager::GetInstance().LoadImg(OBJECT, 10, 12, Path + "object.bmp");		//画像読み込み
	ImageManager::GetInstance().LoadImg(HAIKEI, Path + "haikei.bmp");				//画像読み込み
	ImageManager::GetInstance().LoadImg(HAIKEI2, Path + "haikei1_2.bmp");			//画像読み込み
	ImageManager::GetInstance().LoadImg(SCORE, 16, 16, 10, 2, Path + "score.bmp");			//画像読み込み
}

void Action::LoadSound(INIDat *SDList) {
	std::string SEPath = "dat/SE/";
	SoundManager::GetInstance().SetSE(THREAD, SEPath + SDList->GetData("SEData","THREAD")[0]);
	SoundManager::GetInstance().SetSE(COIN, SEPath + SDList->GetData("SEData", "COIN")[0]);
	SoundManager::GetInstance().SetSE(JUMP1, SEPath + SDList->GetData("SEData", "JUMP1")[0]);
	SoundManager::GetInstance().SetSE(JUMP2, SEPath + SDList->GetData("SEData", "JUMP2")[0]);
	SoundManager::GetInstance().SetSE(JUMP3, SEPath + SDList->GetData("SEData", "JUMP3")[0]);
	SoundManager::GetInstance().SetSE(DAMAG, SEPath + SDList->GetData("SEData", "DAMAGE")[0]);
	SoundManager::GetInstance().SetSE(LADDER, SEPath + SDList->GetData("SEData", "LADDER")[0]);
	SoundManager::GetInstance().SetSE(EXPOSE, SEPath + SDList->GetData("SEData", "EXPOSE")[0]);
	SoundManager::GetInstance().SetSE(HIT, SEPath + SDList->GetData("SEData","HIT")[0]);
	SoundManager::GetInstance().SetSE(PIPE, SEPath + SDList->GetData("SEData", "PIPE")[0]);
	SoundManager::GetInstance().SetSE(DEAD, SEPath + SDList->GetData("SEData", "LOSE")[0]);
	SoundManager::GetInstance().SetSE(BEAT, SEPath + SDList->GetData("SEData", "BEAT")[0]);
	SoundManager::GetInstance().SetSE(EXTEND, SEPath + SDList->GetData("SEData", "EXTEND")[0]);
	SoundManager::GetInstance().SetSE(BROKEN, SEPath + SDList->GetData("SEData", "BROKEN")[0]);
	SoundManager::GetInstance().SetSE(HITBLK, SEPath + SDList->GetData("SEData", "HITBLK")[0]);
	SoundManager::GetInstance().SetSE(DAPBLK, SEPath + SDList->GetData("SEData", "DAPBLK")[0]);
	SoundManager::GetInstance().SetSE(SWIT, SEPath + SDList->GetData("SEData", "SWIT")[0]);
 }

void Action::LoadSound() {
	std::string SEPath = "dat/SE/";
	SoundManager::GetInstance().SetSE(THREAD, SEPath + "fumu.wav");
	SoundManager::GetInstance().SetSE(COIN, SEPath + "coin.wav");
	SoundManager::GetInstance().SetSE(JUMP1, SEPath + "jump1.wav");
	SoundManager::GetInstance().SetSE(JUMP2, SEPath + "jump2.wav");
	SoundManager::GetInstance().SetSE(JUMP3, SEPath + "jump3.wav");
	SoundManager::GetInstance().SetSE(DAMAG, SEPath + "damage.wav");
	SoundManager::GetInstance().SetSE(LADDER, SEPath + "lad.wav");
	SoundManager::GetInstance().SetSE(EXPOSE, SEPath + "app.wav");
	SoundManager::GetInstance().SetSE(HIT, SEPath + "hit.wav");
	SoundManager::GetInstance().SetSE(PIPE, SEPath + "pipe.wav");
	SoundManager::GetInstance().SetSE(DEAD, SEPath + "lose.mp3");
	SoundManager::GetInstance().SetSE(BEAT, SEPath + "beat.mp3");
	SoundManager::GetInstance().SetSE(EXTEND, SEPath + "extend.wav");
	SoundManager::GetInstance().SetSE(BROKEN, SEPath + "broken.wav");
	SoundManager::GetInstance().SetSE(HITBLK, SEPath + "block.wav");
	SoundManager::GetInstance().SetSE(DAPBLK, SEPath + "block2.wav");
	SoundManager::GetInstance().SetSE(SWIT, SEPath + "switch.wav");
	SoundManager::GetInstance().SetSE(WATER, SEPath + "water.wav");
}

void Action::LoadBGM(INIDat *SDList) {
	std::string BGMInfoPath = "dat/BGM/BGMinfo.inf";
	std::string BGMNum = SDList->GetData("BGMData", "BGM")[0];
	std::string BGMPath = "dat/BGM/";
	auto MusicDat = std::make_unique<INIDat>(BGMInfoPath);
	double LoopPoint = 0;
	if (MusicDat->CheckSec(BGMNum) && MusicDat->CheckElem(BGMNum,"BGM")) {
		BGMPath += MusicDat->GetData(BGMNum, "BGM")[0];
		LoopPoint = std::stoi(MusicDat->GetData(BGMNum, "BGMLoopPoint")[0]);
	} else {
		BGMPath += "BGM1.wav";
		LoopPoint = 6595;
	}
	SoundManager::GetInstance().SetBGM(BGM1, LoopPoint, BGMPath);
}

void Action::LoadMapData(int StageNum, INIDat *SDList){
	std::string Path = "dat/stage/" + std::to_string(StageNum) + "/";


	//GetMapData
	std::string Stage = Path + "map" + std::to_string(StageDetailNum) + ".ary";
	std::string StageImg = Path + "img" + std::to_string(StageDetailNum) + ".ary";

	int Time = 0;
	if (!SDList->CheckElem("StageData","ScrollMode"))ScrollMode = 0;
	else ScrollMode =stoi(SDList->GetData("StageData", "ScrollMode")[0]);
	if (!SDList->CheckElem("StageData", "Time"))Time = 0;
	else Time = stoi(SDList->GetData("StageData", "Time")[0]);
	if (SDList->CheckElem("StageData", "StageMoving")){
		std::vector<std::string>tmp = SDList->GetData("StageData", "StageMoving");
		for (auto i = 0; i <tmp.size(); i++)StageMove.push_back(stoi(tmp[i]));
	}
	if(Time != 0 && !StartF)GameData::GetInstance().SetTime(Time);
	StartF = true;


	//ImageData
	if (SDList->CheckSec("ImgDat"))LoadImg(SDList);
	else LoadImg();

	//BGM
	LoadBGM(SDList);


	//SE
	if (SDList->CheckSec("SEData"))LoadSound(SDList);
	else LoadSound();

	//LoadMapData
	PlayerManager::GetInstance().InitPlayer();
	M = std::make_unique<Map>(Stage, StageImg);

	//Initialize Position
	Appear = 0;
	if (SDList->CheckSec("AppearData")) {
		std::string Str = std::to_string(PrevDetailNum);
		if (SDList->CheckElem("AppearData", Str)) {
			std::vector<std::string> tmp = SDList->GetData("AppearData",Str);
			Appear     = std::stoi(tmp[0]);
			float tmpx = std::stof(tmp[1]);
			float tmpy = std::stof(tmp[2]);
			if (tmpx != -1 && tmpy != -1)PlayerManager::GetInstance().Init(tmpx*MAPSIZEX, tmpy*MAPSIZEY);
		}
	}
	if (Appear != 0)PlayerManager::GetInstance().Emerge(10 + Appear);
	M->SetScreenLU(PlayerManager::GetInstance().GetPlayerX(), PlayerManager::GetInstance().GetPlayerY());
	M->SetScrollMode(ScrollMode);
	SoundManager::GetInstance().PlayBGM(BGM1);
	FadeInF = true;
	FadeTime = 0;
}

void Action::DeleteAll() {
	ObjectManager::GetInstance().DeleteAll();
	ImageManager::GetInstance().DeleteAll();
	SoundManager::GetInstance().StopBGM(BGM1);
	InitSoundMem();
}

Action::Action() {}

Action::Action(int StageNum,INIDat* STDat) {
	StartF = false;
	FadeTime = 0;
	FadeInF = false;
	FadeOutF = false;
	FadeF = false;
	Moving = false;
	PrevDetailNum = 0;
	StageDetailNum = 0;
	BeatLevelTime = 0;
	StageNumber = StageNum;
	LoadMapData(StageNum,STDat);
	GameData::GetInstance().InitHiddenTime();
}

void Action::Move(int Num) {
	DeleteAll();
	PrevDetailNum = StageDetailNum;
	StageDetailNum = Num;
	std::unique_ptr<INIDat> SDList(LoadStageData(StageNumber, StageDetailNum));
	LoadMapData(StageNumber, SDList.get());
}

int Action::GetMove() { return PlayerManager::GetInstance().GetMove(); }
int Action::GetMoveTime() { return PlayerManager::GetInstance().GetMoveTime(); }

//移動時更新
void Action::MovingUpdate() {
	if (GetMove() == -1) {
		int tmp = PlayerManager::GetInstance().GetPlayerX();
		Move(StageMove[tmp / (MAPSIZEX * 10)]);
		Sleep(250);
	} else {
		PlayerManager::GetInstance().Move(M.get());
		if (GetMove() == -1) {
			FadeOutF = true;
			FadeTime = 255;
		}
	}
}

//通常時更新
void Action::NormalUpdate() {
	PlayerManager::GetInstance().update(M.get());
	if(!PlayerManager::GetInstance().GetDead())ObjectManager::GetInstance().update(M.get());
	//画面の設定
	if (!PlayerManager::GetInstance().GetDead())M->SetScreenLU(PlayerManager::GetInstance().GetPlayerX(), PlayerManager::GetInstance().GetPlayerY());
	if (!PlayerManager::GetInstance().GetDead())M->update();
	if (!PlayerManager::GetInstance().GetDead())ObjectManager::GetInstance().CollPlyObj();
	GameData::GetInstance().InitHiddenF();
}

//死亡時更新
void Action::DeadUpdate() {
	PlayerManager::GetInstance().update(M.get());
}

bool Action::update() {
	if (!FadeInF && !FadeOutF) {
		if (GetMove() == 0 && !FadeF) {
			if (!PlayerManager::GetInstance().GetDead())NormalUpdate();
			if (PlayerManager::GetInstance().GetDead())DeadUpdate();			
			GetBeatLevel();
		} else {
			MovingUpdate();
		}
	}
	GameData::GetInstance().update();
	return true;
}

void Action::draw() {
	if (FadeInF)FadeIn();
	if (FadeOutF)FadeOut();

	int Imx, Imy;
	ImageManager::GetInstance().GetSize(HAIKEI,Imx,Imy);
	float tmpx = M->GetSLU().x, tmpy = M->GetSLU().y;
	int L = WINDOWX / Imx + 2;
	for (int i = 0; i < L; i++)
		ImageManager::GetInstance().DrawImg(-(((int)tmpx / 16) % Imx) + Imx * i, 0, HAIKEI, FALSE);		//背景
	for (int i = 0; i < L; i++)
		ImageManager::GetInstance().DrawImg(-(((int)tmpx / 8) % Imx) + Imx * i , 0, HAIKEI2, TRUE);		//背景
	if (GetMove() != 0)PlayerManager::GetInstance().draw(M.get());									//主人公描画
	M->draw();																					//マップ描画
	ObjectManager::GetInstance().draw(M.get());														//オブジェクト描画
	if(GetMove() == 0)PlayerManager::GetInstance().draw(M.get());										//主人公描画
	GameData::GetInstance().draw();
}

Action::~Action() {
	DeleteAll();
	InitSoundMem();
}

void Action::FadeIn() {
	SetBright(FadeTime);
	FadeTime += 32;
	if (FadeTime >= 255) {
		FadeInF = false;
		FadeF = false;
		SetBright(255);
		FadeTime = 0;
	}
}

void Action::FadeOut() {
	SetBright(FadeTime);
	FadeTime -= 32;
	if (FadeTime < 0) {
		FadeOutF = false;
		FadeF = false;
		SetBright(0);
		FadeTime = 0;
	}
}

int Action::GetBeatLevel() {
	if (PlayerManager::GetInstance().GetDead()) {
		BeatLevelTime++;
		if (BeatLevelTime >= 128) {
			LevelChanger::GetInstance().Change(WMAPS);
			PlayerManager::GetInstance().ResetHP();
			PlayerManager::GetInstance().PlusZanki(-1);
			ObjectManager::GetInstance().DeleteAll();
			SoundManager::GetInstance().StopBGM(BGM1);
			InitSoundMem();
		}
	}
	if (PlayerManager::GetInstance().GetBeat()) {
		BeatLevelTime++;
		if (BeatLevelTime == 1) {
			SoundManager::GetInstance().StopBGM(BGM1);
			SoundManager::GetInstance().PlaySE(BEAT);
			ObjectManager::GetInstance().KillAll(M.get());
		}
		if (BeatLevelTime >= 128) {
			LevelChanger::GetInstance().Change(WMAPS);
			GameData::GetInstance().SetBeatLevel(StageNumber, 1);
			ObjectManager::GetInstance().DeleteAll();
			InitSoundMem();
		}
	}
	return 0;
}
