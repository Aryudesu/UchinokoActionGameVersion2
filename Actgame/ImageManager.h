#pragma once
#include "Singleton.h"
#include "Dxlib.h"
#include <vector>
#include <string>

#define MAIN_CHARA	0
#define MAINSHOOT	1
#define ENEMY		2
#define ENEMYN		12
#define ENEMYSHOOT	3

#define ITEM		6
#define EFFECT		7
#define SCORE		9
#define MAP1		10
#define MAP2		11
#define MAPOBJ		12
#define OBJECT		13
#define HAIKEI		14
#define HAIKEI2		15
#define TITLE		16

#define WMAPTILE    19
#define WMAPPOINT   20

//画像管理用クラス
//とりあえず領域確保と画像読み込み、描画、データ消しまで
class ImageManager : public Singleton<ImageManager> {
private:
	std::vector<std::vector<int>> img;
public:
	friend class Singleton < ImageManager >;

public:
	//コンストラクタ
	ImageManager();

	//透過色設定
	void SetTrans(int R, int G, int B);

	//画像読み込み
	//ID : オブジェクトID
	//sizex,sizey　縦横切り取るピクセルサイズ
	//CutX,CutY カット数
	//FileName　画像ファイル名
	void LoadImg(int ID, int sizex, int sizey, int CutX, int CutY, const std::string& FileName);

	//画像読み込み
	//ID : オブジェクトID
	//CutX,CutY カット数
	//FileName　画像ファイル名
	void LoadImg(int ID, int CutX, int CutY, const std::string& FileName);

	//画像読み込み
	//ID : オブジェクトID
	//FileName　画像ファイル名
	void LoadImg(int ID, const std::string& FileName);

	//画像サイズ
	void GetSize(int ID, int num, int &width, int &height);
	void GetSize(int ID, int &width, int &height);

	//IDのオブジェクトの画像破棄
	void DestroyImg(int ID);

	//画像描画
	//x,y　画面位置
	//ID,num　オブジェクトIDと描画番号
	//TransFlag　透過するかしないか　TRUE/FALSE
	void DrawImg(float x, float y, int ID, int num, int TransFlag, int TurnY = FALSE);

	//画像描画（0,0に描画）
	//ID　オブジェクトID
	//TransFlag　透過するかしないか　TRUE/FALSE
	void DrawImg(int ID, int TransFlag);

	//画像描画
	//x,y　画面位置
	//ID,num　オブジェクトIDと描画番号
	//TransFlag　透過するかしないか　TRUE/FALSE
	//TurnY　上下反転
	void DrawImg(float x, float y, int ID, int TransFlag);

	void DeleteAll();
};
