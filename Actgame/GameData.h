#pragma once
#include "DxLib.h"
#include "Singleton.h"
#include "Conf.h"
#include <vector>

//残機やスコア等のデータ保持用
class GameData : public Singleton<GameData>{
private:
	int Zanki;		//残機
	int Score;		//スコア
	int DispScore;	//表示用スコア
	int time;		//残り時間
	int HiddenTime;	//出たり消えたりするブロック用
	bool HiddenF;
	bool ONOFF;
	bool ONOFF_F;
	const int TimeBlock = 60;
	std::vector<int> BeatLevel;
	VECTOR WMPos;		//WorldMapの自機の位置
	
public:
	GameData();
	void SetWMPos(float x, float y);
	VECTOR GetWMPos();

	void DataSave();
	void DataLoad();

	void update();
	void draw();

	void InitHiddenTime();
	int  GetHiddenTime();

	void PlusZanki(int num);
	int GetZanki();
	void SetZanki(int num);

	void SetTime(int num);
	void PlusTime(int num);

	void PlusScore(int num);

	void SetBeatLevel(int Num,int Param = 1);
	int GetBeatLevel(int Num);

	void HiddenSE();
	void InitHiddenF();

	void ChangeONOFF();
	bool GetONOFF();
};