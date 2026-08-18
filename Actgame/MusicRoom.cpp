#include "MusicRoom.h"
#include "Conf.h"
#include "Dxlib.h"
#include "InputKey.h"
#include "SoundManager.h"
#include "SceneChanger.h"
#include "LoadIni.h"

void MusicRoom::PlayMusic(int n) {
	std::string BGMPath = "dat/BGM/" + FileName[n];
	if (Play) {
		StopSoundMem(SoundHandle);
		DeleteSoftSound(SoftSoundHandle);
	}
	Play = true;
	PlayNum = n;
	StrX = WINDOWX;
	SoundHandle = SoundManager::GetInstance().SetSSBGM(BGM1, LoopPoint[n], BGMPath);
	SoftSoundHandle = SoundManager::GetInstance().GetSoftSoundHandle();
	SoundManager::GetInstance().PlaySSBGM(BGM1);
}

void MusicRoom::update() {
	StrX -= 2;
	//下押した時
	if (ReturnKey(KEY_INPUT_DOWN) % 10 == 1) {
		Select = (Select + 1) % Title.size();
	}//上押した時
	else if (ReturnKey(KEY_INPUT_UP) % 10 == 1) {
		Select = (Select + Title.size() - 1) % Title.size();
	} else if (ReturnKey(KEY_INPUT_RIGHT) % 10 == 1) {
		Row++;
		if (Row * 10 >= Title.size())Row--;
		if (Row * 10 + Select >= Title.size())Select = Title.size() % 10;
	} else if (ReturnKey(KEY_INPUT_LEFT) % 10 == 1) {
		Row--;
		if (Row < 0)Row = 0;
	}
	if (ReturnKey(KEY_INPUT_Z) == 1)PlayMusic(Select);


	if (ReturnKey(KEY_INPUT_X) == 1) {
		if (Play)SoundManager::GetInstance().StopBGM(BGM1);
		Play = false;
		StrX = WINDOWX;
	}
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		if (Play)SoundManager::GetInstance().StopBGM(BGM1);
	}
}

void MusicRoom::draw() {
	if (Play)FFT();
	for (int i = 0; 10 * Row + i < Title.size() && i < 10; i++) {
		DrawString(50, 32 + i * (16 + 8), ListTitle[10*Row + i].c_str(), GetColor(255, 255, 255));
	}
	DrawString(20, 32 + Select * (16+8), "■", GetColor(255, 255, 255));
	DrawBox(0, WINDOWY - 32, WINDOWX, WINDOWY, GetColor(64, 64, 255), TRUE);
	std::string str;
	if (!Play)str = "ここに説明文が表示されます";
	else str = Title[PlayNum] + "  :  " + Auther[PlayNum];
	if (StrX < -GetDrawStringWidth(str.c_str(), str.size()))StrX = WINDOWX;
	DrawString(StrX,WINDOWY - 24, str.c_str(), GetColor(255, 255, 255));
}

void MusicRoom::InputDat() {
	INIDat Dat = INIDat("dat/BGM/BGMInfo.inf");
	for (int i = 1;; i++) {
		std::string sec = std::to_string(i);
		if (!Dat.CheckSec(sec))break;
		ListTitle.push_back(Dat.GetData(sec, "LISTTITLE")[0]);
		Title.push_back(Dat.GetData(sec,"BGMTITLE")[0]);
		Auther.push_back(Dat.GetData(sec, "Auther")[0]);
		LoopPoint.push_back(std::stoi(Dat.GetData(sec, "BGMLoopPoint")[0]));
		FileName.push_back(Dat.GetData(sec, "BGM")[0]);
	}
	RowMax = (Title.size() - 1) / 10;
}

MusicRoom::MusicRoom() {
	InputDat();
	StrX = WINDOWX;
}

void MusicRoom::FFT() {
	int SamplePos;
	const int BUFFERLENGTH = 16384;
	float ParamList[BUFFERLENGTH];
	SamplePos = GetCurrentPositionSoundMem(SoundHandle);
	GetFFTVibrationSoftSound(SoftSoundHandle, -1, SamplePos, 4096 * 2, ParamList, BUFFERLENGTH);
	int x = -1, j = 0;
	for (int i = 0; i < BUFFERLENGTH; i++){
		if ((int)(log10((double)i) * 10) != x) {
			j++;
			x = (int)(log10((double)i) * 10);
			float Param;
			//関数から取得できる値を描画に適した値に調整
			if (ParamList[i] > 0.000000000001f){
				Param = log(ParamList[i]) * 10.0f;
			} else {
				Param = -120.0f;
			}
			//縦線を描画
			int a = 6,b = WINDOWX/32;
			DrawBox((j-a) * b, WINDOWY-32 - (int)((120.0f + Param) / 120.0f * (WINDOWY-32)), (j-a) * b + b-1, WINDOWY-32, GetColor(16, 32, 16), TRUE);
		}
	}
}