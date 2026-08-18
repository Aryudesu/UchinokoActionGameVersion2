#include "GameData.h"
#include "ImageManager.h"
#include "PlayerManager.h"
#include "SoundManager.h"
#include "function.h"
#include "Cipher.h"

GameData::GameData() {
	Zanki = 5;
	WMPos.x = -1;
	WMPos.y = -1;
	BeatLevel.resize(1500);
	Zanki = 5;
	Score = 0;
	PlayerManager::GetInstance().SetHP(3);
	PlayerManager::GetInstance().SetCoin(0);
	for (int i = 0; i < BeatLevel.size(); i++)BeatLevel[i] = 0;
	HiddenTime = 0;
	HiddenF = false;
	ONOFF = true;
	ONOFF_F = false;
}

void GameData::SetWMPos(float x, float y) {
	WMPos.x = x;
	WMPos.y = y;
}

VECTOR GameData::GetWMPos() { return WMPos; }

void GameData::SetBeatLevel(int Num,int Param){ BeatLevel[Num] = Param; }
int GameData::GetBeatLevel(int Num) { return BeatLevel[Num]; }

void GameData::update() {
	HiddenTime++;
	if(PlayerManager::GetInstance().GetMove() == 0 && !PlayerManager::GetInstance().GetDead() && !PlayerManager::GetInstance().GetBeat())time--;
	if (time == 0)PlayerManager::GetInstance().Death();
}

void GameData::SetTime(int num) { time = num * TimeBlock; }
void GameData::PlusTime(int num) { time += (num <= 999) ? num * TimeBlock : 999 * TimeBlock; }

void GameData::PlusScore(int num) { Score += num; }

void GameData::PlusZanki(int num) { Zanki += num; if (Zanki > 999)Zanki = 999; }
int GameData::GetZanki() { return Zanki; }
void GameData::SetZanki(int num) { Zanki = num; if (Zanki > 999)Zanki = 999; }

void GameData::InitHiddenTime() { HiddenTime = 0; }
int  GameData::GetHiddenTime() { return HiddenTime; }

void GameData::draw() {
	ImageManager::GetInstance().DrawImg(10, 10, SCORE, 6, TRUE);
	ImageManager::GetInstance().DrawImg(26, 10, SCORE, 7, TRUE);
	ImageManager::GetInstance().DrawImg(42, 10, SCORE, 8, TRUE);
	//スコア
	if (DispScore < Score) {
		DispScore += (Score - DispScore) / 7;
		if (Score - DispScore < 7)DispScore = Score;
	}
	for (int i = 0, num = DispScore; i < 10; i++, num /= 10)ImageManager::GetInstance().DrawImg(64 - i*11+84, 10, SCORE, num%10 + 10, TRUE);
	//タイム
	for (int i = 0, num = (time >= 0) ? (time + TimeBlock - 1) / TimeBlock : 0; i < 3; i++, num /= 10)ImageManager::GetInstance().DrawImg(WINDOWX / 2 - 5 - i * 11, 10, SCORE, num % 10 + 10, TRUE);
	//コイン
	int Coin = PlayerManager::GetInstance().GetCoin();
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 - 11, 30, SCORE, 4, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 +  5, 30, SCORE, 3, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 + 28, 30, SCORE, 10 + Coin / 10, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 + 39, 30, SCORE, 10 + Coin % 10, TRUE);
	//残基
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 - 11, 10, SCORE, 2, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 +  5, 10, SCORE, 3, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 + 17, 10, SCORE, 10 +  Zanki / 100, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 + 28, 10, SCORE, 10 + (Zanki / 10) % 10, TRUE);
	ImageManager::GetInstance().DrawImg(WINDOWX - 3 * 16 - 14 + 39, 10, SCORE, 10 +  Zanki % 10, TRUE);
	//HP
	int HP = PlayerManager::GetInstance().GetHP();
	for (int i = 0; i < HP; i++)ImageManager::GetInstance().DrawImg(24 + i * 8, 28, SCORE, 0, TRUE);
	//クリアしたとき
	if (PlayerManager::GetInstance().GetBeat()) {
		int StrLen, StrWidth;
		std::string Mes = "Stage Clear!!";
		StrLen = (int)strlen(Mes.c_str());
		StrWidth = GetDrawStringWidth(Mes.c_str(), StrLen);
		DrawFormatString((WINDOWX - StrWidth) / 2, WINDOWY / 2, GetColor(255, 255, 255), Mes.c_str());
	}
}

void GameData::DataSave() {
	std::string Str,Tmp;
	int Key = Zanki + PlayerManager::GetInstance().GetHP() + Score + PlayerManager::GetInstance().GetCoin() + (int)(WMPos.x * 100) + (int)(WMPos.y * 100);
	Str = "" + std::to_string(Zanki) + "," + std::to_string(PlayerManager::GetInstance().GetHP()) + "," + std::to_string(Score) + "," + std::to_string(PlayerManager::GetInstance().GetCoin()) + "," + std::to_string((int)(WMPos.x * 100)) + "," + std::to_string((int)(WMPos.y * 100));
	Tmp = "";
	for (int i = 0; i < BeatLevel.size(); i++) {
		Tmp += "," + std::to_string(BeatLevel[i]);
		Key += BeatLevel[i];
	}
	Str += Tmp;
	GameEnc(Str, "SaveData.dat", Key);
}

void GameData::DataLoad() {
	FILE *fp;
	if (fopen_s(&fp, "SaveData.dat", "r") != 0)return;
	fclose(fp);
	std::string str = GameDec("SaveData.dat");
	//Message(str.c_str());
	std::vector<int> Dat = SplitNum(str,',');
	int Key1 = Dat[0], Key2 = Dat[1], Key3 = 0;
	Dat.erase(Dat.begin());
	Dat.erase(Dat.begin());
	for (int i = 0; i < Dat.size(); i++)Key3 += Dat[i];
	if (Key3 % Key2 != Key1) {
		Message("セーブデータがなんかおかしい");
		exit(0);
	}
	Zanki = Dat[0];
	PlayerManager::GetInstance().SetHP(Dat[1]);
	DispScore = Score = Dat[2];
	PlayerManager::GetInstance().SetCoin(Dat[3]);
	WMPos.x = (float)Dat[4] / 100.;
	WMPos.y = (float)Dat[5] / 100.;
	for (int i = 6; i < BeatLevel.size(); i++)BeatLevel[i-6] = Dat[i];
}

void GameData::HiddenSE() {
	if (!HiddenF) { SoundManager::GetInstance().PlaySE(DAPBLK); }
}

void GameData::InitHiddenF() {
	HiddenF = false;
	ONOFF_F = false;
}

void GameData::ChangeONOFF() {
	if (!ONOFF_F) {
		ONOFF = !ONOFF;
		SoundManager::GetInstance().PlaySE(SWIT);
	}
	ONOFF_F = true;
}

bool GameData::GetONOFF() {
	return ONOFF;
}