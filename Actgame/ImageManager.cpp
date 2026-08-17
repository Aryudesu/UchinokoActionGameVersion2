#include "ImageManager.h"
#include "Dxlib.h"
#include <vector>
#include <string>

//コンストラクタ
ImageManager::ImageManager() { img.resize(32); }

//透過色設定
void ImageManager::SetTrans(int R, int G, int B) { SetTransColor(R, G, B); }

//画像読み込み
//ID : オブジェクトID
//sizex,sizey　縦横切り取るピクセルサイズ
//CutX,CutY カット数
//FileName　画像ファイル名
void ImageManager::LoadImg(int ID, int sizex, int sizey, int CutX, int CutY, std::string FileName) {
	img[ID].resize(CutX*CutY);
	LoadDivGraph(FileName.c_str(), CutX*CutY, CutX, CutY, sizex, sizey, &img[ID][0]);
}

//画像読み込み
//ID : オブジェクトID
//CutX,CutY カット数
//FileName　画像ファイル名
void ImageManager::LoadImg(int ID, int CutX, int CutY, std::string FileName) {
	img[ID].resize(CutX*CutY);
	LoadDivGraph(FileName.c_str(), CutX*CutY, CutX, CutY, 32, 32, &img[ID][0]);
}

//画像読み込み
//ID : オブジェクトID
//FileName　画像ファイル名
void ImageManager::LoadImg(int ID, std::string FileName) {
	img[ID].resize(1);
	img[ID][0] = LoadGraph(FileName.c_str());
}

//画像サイズ
void ImageManager::GetSize(int ID, int num, int &width, int &height) {
	int x, y;
	GetGraphSize(img[ID][num],&x,&y);
	width  = x;
	height = y;
}

//画像サイズ
void ImageManager::GetSize(int ID,int &width, int &height) {
	int x, y;
	GetGraphSize(img[ID][0], &x, &y);
	width = x;
	height = y;
}


//IDのオブジェクトの画像破棄
void ImageManager::DestroyImg(int ID) {
	for (int i = 0; i < img[ID].size(); i++) {
		DeleteGraph(img[ID][i]);
	}
}

//画像描画
//x,y　画面位置
//ID,num　オブジェクトIDと描画番号
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::DrawImg(float x, float y, int ID, int num, int TransFlag,int TurnY) { DrawRotaGraph2(x, y,0,0, 1, 0, img[ID][num], TransFlag, FALSE, TurnY); }

//画像描画（0,0に描画）
//ID　オブジェクトID
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::DrawImg(int ID, int TransFlag) { DrawGraph(0, 0, img[ID][0], TransFlag); }

//画像描画
//x,y　画面位置
//ID,num　オブジェクトIDと描画番号
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::DrawImg(float x, float y, int ID, int TransFlag) { DrawGraph(x, y, img[ID][0], TransFlag); }


void ImageManager::DeleteAll() {
	InitGraph();
	for (int i = 0; i < img.size(); i++) {
		for (int j = 0; j < img[i].size(); j++) {
			img[i].erase(img[i].begin() + j);
			img[i].clear();
		}
	}
	img.clear();
	img.resize(32);
}