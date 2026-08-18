#pragma once
#include "Block.h"
#include "DxLib.h"
#include "Singleton.h"
#include <vector>
#include <string>

//ステージマップデータ
class Map : Singleton<Map>{
private:
	VECTOR Size;
	VECTOR ScreenLU;
	std::vector<std::vector<Block*>> dat;
	float SX, SY;
	int ScrollMode;

	//マップ読み込み
	void LoadMap(std::string FileName);

	//マップの画像チップデータ読み込み
	void SetMapChip(std::string FileName);

	void SetSize(std::vector<std::vector <int>> Dat);

public:

	void SetScreenLU(float x, float y);

	float GetScreenLUX();
	float GetScreenLUY();

	Map(std::string FileName, std::string FileName2);

	//マップ更新
	void update();

	//描画
	void draw();

	float GetSX();
	float GetSY();

	//ブロックをセット
	void SetBlock(int num, int x, int y);

	//広さを知りたい
	VECTOR GetWorldSize();

	//ブロック番号
	int GetNum(int x, int y);
	//攻撃判定番号
	int GetKill(int x, int y);

	//ブロック叩かれた判定発生したとき
	void Hited(int x, int y);

	//触れた時
	void Touched(int x, int y);

	//スクロールモード
	void SetScrollMode(int Num);
	int GetScrollMode();

	//画面左上のワールド座標
	VECTOR GetSLU();

	void DeleteAll();

	~Map();
};