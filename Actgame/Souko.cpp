#include "Souko.h"
#include "GameData.h"
#include "SoundManager.h"
#include "function.h"
#include "InputKey.h"
#include "ImageManager.h"
#include "LevelChanger.h"
/*
-1:壁
 0:空白
 3:普通荷物
 4:ポイントの上荷物
 5:ポイント
*/

void Souko::Reset() {
	Map = ResetMap;
	for (int x = 0; x < Map[0].size(); x++) {
		for (int y = 0; y < Map.size(); y++) {
			if (Map[y][x] == 1 || Map[y][x] == 2) {
				sx = x;
				sy = y;
				if (Map[y][x] == 1)Map[y][x] = 0;
				if (Map[y][x] == 2)Map[y][x] = 5;
			}
		}
	}
}

void Souko::LoadMapData(int StageNum, INIDat* SDList) {
	std::string Path = "dat/stage/" + std::to_string(StageNum) + "/";
	//GetMapData
	std::string Stage = Path + "map" + std::to_string(StageDetailNum) + ".ary";
	std::vector<std::vector<int>> tmp = LoadArray(Stage);
	std::vector<int> tmp3;
	for (int i = 0; i < tmp[0].size() + 2; i++)tmp3.push_back(-1);
	Map.push_back(tmp3);
	for (int y = 0; y < tmp.size(); y++) {
		std::vector<int> tmp2;
		tmp2.push_back(-1);
		for (int x = 0; x < tmp[0].size(); x++) {
			tmp2.push_back(tmp[y][x]);
		}
		tmp2.push_back(-1);
		Map.push_back(tmp2);
	}
	Map.push_back(tmp3);
	ResetMap = Map;

	for (int x = 0; x < Map[0].size(); x++) {
		for (int y = 0; y < Map.size(); y++) {
			if (Map[y][x] == 1 || Map[y][x] == 2) {
				sx = x;
				sy = y;
				if (Map[y][x] == 1)Map[y][x] = 0;
				if (Map[y][x] == 2)Map[y][x] = 5;
			}
		}
	}

	//BGM
	LoadBGM(SDList);
	//SE
	LoadSound();
}

void Souko::LoadSound() {
	std::string SEPath = "dat/SE/";
	SoundManager::GetInstance().SetSE(JUMP1, SEPath + "Aruku.wav");
	SoundManager::GetInstance().SetSE(HIT, SEPath + "Suru.wav");
	SoundManager::GetInstance().SetSE(BEAT, SEPath + "beat.mp3");
	SoundManager::GetInstance().SetSE(SWIT, SEPath + "Reset.wav");
}

void Souko::LoadBGM(INIDat* SDList) {
	std::string BGMInfoPath = "dat/BGM/BGMinfo.inf";
	std::string BGMNum = SDList->GetData("BGMData", "BGM")[0];
	std::string BGMPath = "dat/BGM/";
	INIDat* MusicDat = new INIDat(BGMInfoPath);
	double LoopPoint = 0;
	if (MusicDat->CheckSec(BGMNum) && MusicDat->CheckElem(BGMNum, "BGM")) {
		BGMPath += MusicDat->GetData(BGMNum, "BGM")[0];
		LoopPoint = std::stoi(MusicDat->GetData(BGMNum, "BGMLoopPoint")[0]);
	}
	else {
		BGMPath += "BGM1.wav";
		LoopPoint = 6595;
	}
	SoundManager::GetInstance().SetBGM(BGM1, LoopPoint, BGMPath);
}

Souko::~Souko() {
	ImageManager::GetInstance().DeleteAll();
	SoundManager::GetInstance().StopBGM(BGM1);
	InitSoundMem();
}

Souko::Souko() {
	StageDetailNum = 0;
	BeatLevelTime = 0;
	StageNumber = 0;
	Step = 0;
	StepMax = 0;
	sx = 0;
	sy = 0;
	movetime = 0;
	direx = direy = 0;
	animetime = 0;
	move = false;
	Beat = false;
	BeatTime = 0;
}

Souko::Souko(int StageNum, INIDat* STDat) {
	StageDetailNum = 0;
	BeatLevelTime = 0;
	StageNumber = StageNum;
	move = false;
	direx = direy = 0;
	animetime = 0;
	Beat = false;
	BeatTime = 0;
	Bright = 0;
	BrightF = 1;
	LoadMapData(StageNum, STDat);
	GameData::GetInstance().InitHiddenTime();
	LoadImg();
	SoundManager::GetInstance().PlayBGM(BGM1);
}

void Souko::LoadImg(INIDat* SDList) {
	std::string Path = "dat/img/";
	ImageManager::GetInstance().SetTrans(TRANSR, TRANSG, TRANSB);						//透過色指定
	if (SDList->CheckElem("ImgDat", "MainCharactor"))ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 9, Path + SDList->GetData("ImgDat", "MainCharactor")[0]);	//画像読み込み
		else ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 9, Path + "SoukoChara.bmp");	//画像読み込み
	if (SDList->CheckElem("ImgDat", "Block"))ImageManager::GetInstance().LoadImg(MAP1, 1, 5, Path + SDList->GetData("ImgDat", "Block")[0]);
		else ImageManager::GetInstance().LoadImg(MAP1, 1, 5, Path + "SoukoObj.bmp");				//画像読み込み
	if (SDList->CheckElem("ImgDat", "Background"))ImageManager::GetInstance().LoadImg(HAIKEI, Path + SDList->GetData("ImgDat", "Background")[0]);				//画像読み込み
		else ImageManager::GetInstance().LoadImg(HAIKEI, Path + "SoukoHaikei.bmp");
}

void Souko::LoadImg() {
	std::string Path = "dat/img/";
	ImageManager::GetInstance().SetTrans(TRANSR, TRANSG, TRANSB);						//透過色指定
	ImageManager::GetInstance().LoadImg(MAIN_CHARA, 4, 9, Path + "SoukoChara.bmp");	//画像読み込み
	ImageManager::GetInstance().LoadImg(MAP1, 5, 1, Path + "SoukoObj.bmp");				//画像読み込み
	ImageManager::GetInstance().LoadImg(HAIKEI, Path + "SoukoHaikei.bmp");				//画像読み込み
}

void Souko::SCharaMove() {
	if (ReturnKey(KEY_INPUT_UP) != 0 && (ReturnKey(KEY_INPUT_DOWN) + ReturnKey(KEY_INPUT_RIGHT) + ReturnKey(KEY_INPUT_LEFT) == 0)) {
		if (Map[sy - 1][sx] == 0 || Map[sy - 1][sx] == 5) {
			sy--;
			direy = -1;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(JUMP1);
		}
		else if ((Map[sy - 1][sx] == 3 || Map[sy - 1][sx] == 4) && (Map[sy - 2][sx] != -1 && Map[sy - 2][sx] != 3 && Map[sy - 2][sx] != 4)) {
			if (Map[sy - 2][sx] == 5) {
				Map[sy - 2][sx] = 4;
				if (Map[sy - 1][sx] == 3)Map[sy - 1][sx] = 0;
				if (Map[sy - 1][sx] == 4)Map[sy - 1][sx] = 5;
			}
			if (Map[sy - 2][sx] == 0) {
				Map[sy - 2][sx] = 3;
				if (Map[sy - 1][sx] == 3)Map[sy - 1][sx] = 0;
				if (Map[sy - 1][sx] == 4)Map[sy - 1][sx] = 5;
			}
			sy--;
			direy = -2;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(HIT);
		}
	}
	if (ReturnKey(KEY_INPUT_DOWN) != 0 && (ReturnKey(KEY_INPUT_UP) + ReturnKey(KEY_INPUT_RIGHT) + ReturnKey(KEY_INPUT_LEFT) == 0)) {
		if (Map[sy + 1][sx] == 0 || Map[sy + 1][sx] == 5) {
			sy++;
			direy = 1;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(JUMP1);
		}
		else if ((Map[sy + 1][sx] == 3 || Map[sy + 1][sx] == 4) && (Map[sy + 2][sx] != -1 && Map[sy + 2][sx] != 3 && Map[sy + 2][sx] != 4)) {
			if (Map[sy + 2][sx] == 5) {
				Map[sy + 2][sx] = 4;
				if (Map[sy + 1][sx] == 3)Map[sy + 1][sx] = 0;
				if (Map[sy + 1][sx] == 4)Map[sy + 1][sx] = 5;
			}
			if (Map[sy + 2][sx] == 0) {
				Map[sy + 2][sx] = 3;
				if (Map[sy + 1][sx] == 3)Map[sy + 1][sx] = 0;
				if (Map[sy + 1][sx] == 4)Map[sy + 1][sx] = 5;
			}
			sy++;
			direy = 2;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(HIT);
		}
	}
	if (ReturnKey(KEY_INPUT_RIGHT) != 0 && (ReturnKey(KEY_INPUT_DOWN) + ReturnKey(KEY_INPUT_UP) + ReturnKey(KEY_INPUT_LEFT) == 0)) {
		if (Map[sy][sx + 1] == 0 || Map[sy][sx + 1] == 5) {
			sx++;
			direx = 1;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(JUMP1);
		}
		else if ((Map[sy][sx + 1] == 3 || Map[sy][sx + 1] == 4) && (Map[sy][sx + 2] != -1 && Map[sy][sx + 2] != 3 && Map[sy][sx + 2] != 4)) {
			if (Map[sy][sx + 2] == 5) {
				Map[sy][sx + 2] = 4;
				if (Map[sy][sx + 1] == 3)Map[sy][sx + 1] = 0;
				if (Map[sy][sx + 1] == 4)Map[sy][sx + 1] = 5;
			}
			if (Map[sy][sx + 2] == 0) {
				Map[sy][sx + 2] = 3;
				if (Map[sy][sx + 1] == 3)Map[sy][sx + 1] = 0;
				if (Map[sy][sx + 1] == 4)Map[sy][sx + 1] = 5;
			}
			sx++;
			direx = 2;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(HIT);
		}
	}
	if (ReturnKey(KEY_INPUT_LEFT) != 0 && (ReturnKey(KEY_INPUT_DOWN) + ReturnKey(KEY_INPUT_RIGHT) + ReturnKey(KEY_INPUT_UP) == 0)) {
		if (Map[sy][sx - 1] == 0 || Map[sy][sx - 1] == 5) {
			sx--;
			direx = -1;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(JUMP1);
		}
		else if ((Map[sy][sx - 1] == 3 || Map[sy][sx - 1] == 4) && (Map[sy][sx - 2] != -1 && Map[sy][sx - 2] != 3 && Map[sy][sx - 2] != 4)) {
			if (Map[sy][sx - 2] == 5) {
				Map[sy][sx - 2] = 4;
				if (Map[sy][sx - 1] == 3)Map[sy][sx - 1] = 0;
				if (Map[sy][sx - 1] == 4)Map[sy][sx - 1] = 5;
			}
			if (Map[sy][sx - 2] == 0) {
				Map[sy][sx - 2] = 3;
				if (Map[sy][sx - 1] == 3)Map[sy][sx - 1] = 0;
				if (Map[sy][sx - 1] == 4)Map[sy][sx - 1] = 5;
			}
			sx--;
			direx = -2;
			move = true;
			movetime = 0;
			animetime = 0;
			SoundManager::GetInstance().PlaySE(HIT);
		}
	}
}

bool Souko::update() {
	if (BrightF == 1) {
		Bright += 10;
		if (Bright >= 255) {
			BrightF = 0;
			Bright = 255;
		}
	}
	if (BrightF == 2) {
		Bright -= 10;
		if (Bright <= 0) {
			Bright = 0;
			BrightF = 1;
		}
	}
	if (move) {
		movetime++;
		if (movetime == 16) {
			move = false;
			movetime = 0;
			direx = 0;
			direy = 0;
		}
	}
	if(!move)Beat = CheckBeat();
	if (ReturnKey(KEY_INPUT_RETURN) == 1 && !Beat && !move && BrightF == 0) {
		SoundManager::GetInstance().PlaySE(SWIT);
		BrightF = 2;
		ResetF = true;
	}
	if (ResetF && BrightF == 1) {
		Reset();
		ResetF = false;
	}
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1 && !Beat) {
		SoundManager::GetInstance().StopBGM(BGM1);
		LevelChanger::GetInstance().Change(WMAPS);
	}
	if (!move && movetime == 0 && !Beat && BrightF != 2)SCharaMove();
	animetime++;
	if (animetime == 32)animetime = 0;
	if (Beat)BeatTime++;
	GetBeatLevel();
	return true;
}

void Souko::draw() {
	if(BrightF != 0)SetBright(Bright);
	ImageManager::GetInstance().DrawImg(0, 0, HAIKEI, FALSE);
	bool mb = false;
	int mbx = 0, mby = 0;
	if (abs(direx) == 2 || abs(direy) == 2) {
		mb = true;
		mbx = sx;
		mby = sy;
		if (direx ==  2)mbx = sx + 1;
		if (direx == -2)mbx = sx - 1;
		if (direy ==  2)mby = sy + 1;
		if (direy == -2)mby = sy - 1;
	}
	for (int x = 0; x < Map[0].size(); x++) {
		for (int y = 0; y < Map.size(); y++) {
			if (Map[y][x] == -1)ImageManager::GetInstance().DrawImg(x * MAPSIZEX, y * MAPSIZEY, MAP1, 1, TRUE);
			if (Map[y][x] ==  5)ImageManager::GetInstance().DrawImg(x * MAPSIZEX, y * MAPSIZEY, MAP1, 4, TRUE);
			if (!mb || (mb && (mbx != x || mby != y))) {
				if (Map[y][x] == 3)ImageManager::GetInstance().DrawImg(x * MAPSIZEX, y * MAPSIZEY, MAP1, 2, TRUE);
				if (Map[y][x] == 4)ImageManager::GetInstance().DrawImg(x * MAPSIZEX, y * MAPSIZEY, MAP1, 3, TRUE);
			} else {
				if (Map[y][x] == 4)ImageManager::GetInstance().DrawImg(x * MAPSIZEX, y * MAPSIZEY, MAP1, 4, TRUE);
			}
		}
	}
	if(!move)ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, sy * MAPSIZEY, MAIN_CHARA, animetime/8, TRUE);
	if (move) {
		if (direx ==  1)ImageManager::GetInstance().DrawImg((sx - 1) * MAPSIZEX + movetime * 2, sy * MAPSIZEY, MAIN_CHARA, 12 + animetime / 8, TRUE);
		if (direx == -1)ImageManager::GetInstance().DrawImg((sx + 1) * MAPSIZEX - movetime * 2, sy * MAPSIZEY, MAIN_CHARA, 16 + animetime / 8, TRUE);
		if (direy ==  1)ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, (sy - 1) * MAPSIZEY + movetime * 2, MAIN_CHARA,  4 + animetime / 8, TRUE);
		if (direy == -1)ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, (sy + 1) * MAPSIZEY - movetime * 2, MAIN_CHARA,  8 + animetime / 8, TRUE);
		if (direx == 2) {
			ImageManager::GetInstance().DrawImg((sx - 1) * MAPSIZEX + movetime * 2, sy * MAPSIZEY, MAIN_CHARA, 28 + animetime / 8, TRUE);
			ImageManager::GetInstance().DrawImg(sx * MAPSIZEX + movetime * 2, sy * MAPSIZEY, MAP1, 2, TRUE);
		}
		if (direx == -2) {
			ImageManager::GetInstance().DrawImg((sx + 1) * MAPSIZEX - movetime * 2, sy * MAPSIZEY, MAIN_CHARA, 32 + animetime / 8, TRUE);
			ImageManager::GetInstance().DrawImg(sx * MAPSIZEX - movetime * 2, sy * MAPSIZEY, MAP1, 2, TRUE);
		}
		if (direy == 2) {
			ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, (sy - 1) * MAPSIZEY + movetime * 2, MAIN_CHARA, 20 + animetime / 8, TRUE);
			ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, sy * MAPSIZEY + movetime * 2, MAP1, 2, TRUE);
		}
		if (direy == -2) {
			ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, (sy + 1) * MAPSIZEY - movetime * 2, MAIN_CHARA, 24 + animetime / 8, TRUE);
			ImageManager::GetInstance().DrawImg(sx * MAPSIZEX, sy * MAPSIZEY - movetime * 2, MAP1, 2, TRUE);
		}
	}
	if (Beat) {
		int StrLen, StrWidth;
		std::string Mes = "Stage Clear!!";
		StrLen = (int)strlen(Mes.c_str());
		StrWidth = GetDrawStringWidth(Mes.c_str(), StrLen);
		DrawFormatString((WINDOWX - StrWidth) / 2 - 2, WINDOWY / 2, GetColor(0, 0, 0), Mes.c_str());
		DrawFormatString((WINDOWX - StrWidth) / 2 + 2, WINDOWY / 2, GetColor(0, 0, 0), Mes.c_str());
		DrawFormatString((WINDOWX - StrWidth) / 2, WINDOWY / 2 - 2, GetColor(0, 0, 0), Mes.c_str());
		DrawFormatString((WINDOWX - StrWidth) / 2, WINDOWY / 2 + 2, GetColor(0, 0, 0), Mes.c_str());
		DrawFormatString((WINDOWX - StrWidth) / 2, WINDOWY / 2, GetColor(255, 255, 255), Mes.c_str());
	}
}

bool Souko::CheckBeat() {
	for (int x = 0; x < Map[0].size(); x++) {
		for (int y = 0; y < Map.size(); y++) {
			if (Map[y][x] == 3 || Map[y][x] == 5)return false;
		}
	}
	return true;
}

int Souko::GetBeatLevel() {
	if (BeatTime == 1) {
		SoundManager::GetInstance().StopBGM(BGM1);
		SoundManager::GetInstance().PlaySE(BEAT);
	}
	if (BeatTime > 128) {
		LevelChanger::GetInstance().Change(WMAPS);
		GameData::GetInstance().SetBeatLevel(StageNumber, 1);
	}
	return 0;
}