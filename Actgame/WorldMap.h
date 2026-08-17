#pragma once
#include <vector>
#include <string>
#include "Dxlib.h"
#include "GameCollection.h"

class WorldMap : public GameCollection{
private:
	std::vector<std::vector<int>> Data;
	std::vector<std::vector<int>> ImgData;
	VECTOR SCLU;
	VECTOR pos;
	bool GameOverF = false;
	bool Kaiwa = false;
	int GameOverTime = 0;
	int PAnime,PImgBase,PDire;
	void PlayerUpdate();
	void GameOver();
	bool IsBlock(int Num);

	std::vector<std::vector<std::string>> KaiwaDat;
	std::vector<std::vector<std::string>> KaiwaLoad(int Num);
	bool KaiwaExist(int Num);
	void KaiwaEv();
	void KaiwaDraw(int Num);
	void KaiwaInit(int Num = 0);
	int KaiwaLine;
	int KaiwaNum;
	bool Kaiwa_Z;

public:
	WorldMap();
	WorldMap(std::string MapDat, std::string ImgDat);
	~WorldMap();
	bool update();
	void draw();
	int GetBeatLevel() { return 1; };
};