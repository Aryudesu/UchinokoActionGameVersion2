#include "ImageManager.h"
#include "Dxlib.h"
#include <vector>
#include <string>

//コンストラクタ
ImageManager::ImageManager() { img.resize(32); }

bool ImageManager::IsValidImage(int ID, int num) const {
	return ID >= 0 && ID < static_cast<int>(img.size()) &&
		num >= 0 && num < static_cast<int>(img[ID].size()) &&
		img[ID][num] >= 0;
}

//透過色設定
void ImageManager::SetTrans(int R, int G, int B) { SetTransColor(R, G, B); }

//画像読み込み
//ID : オブジェクトID
//sizex,sizey　縦横切り取るピクセルサイズ
//CutX,CutY カット数
//FileName　画像ファイル名
void ImageManager::LoadImg(int ID, int sizex, int sizey, int CutX, int CutY, const std::string& FileName) {
	DestroyImg(ID);
	img[ID].resize(CutX*CutY);
	LoadDivGraph(FileName.c_str(), CutX*CutY, CutX, CutY, sizex, sizey, img[ID].data());
}

//画像読み込み
//ID : オブジェクトID
//CutX,CutY カット数
//FileName　画像ファイル名
void ImageManager::LoadImg(int ID, int CutX, int CutY, const std::string& FileName) {
	DestroyImg(ID);
	img[ID].resize(CutX*CutY);
	LoadDivGraph(FileName.c_str(), CutX*CutY, CutX, CutY, 32, 32, img[ID].data());
}

//画像読み込み
//ID : オブジェクトID
//FileName　画像ファイル名
void ImageManager::LoadImg(int ID, const std::string& FileName) {
	DestroyImg(ID);
	img[ID].resize(1);
	img[ID][0] = LoadGraph(FileName.c_str());
}

//画像サイズ
void ImageManager::GetSize(int ID, int num, int &width, int &height) {
	if (!IsValidImage(ID, num)) {
		// Callers use the size for division/modulo when drawing backgrounds.
		width = 1;
		height = 1;
		return;
	}
	int x, y;
	GetGraphSize(img[ID][num],&x,&y);
	width  = x;
	height = y;
}

//画像サイズ
void ImageManager::GetSize(int ID,int &width, int &height) {
	if (!IsValidImage(ID, 0)) {
		width = 1;
		height = 1;
		return;
	}
	int x, y;
	GetGraphSize(img[ID][0], &x, &y);
	width = x;
	height = y;
}


//IDのオブジェクトの画像破棄
void ImageManager::DestroyImg(int ID) {
	if (ID < 0 || ID >= static_cast<int>(img.size())) return;
	for (const int handle : img[ID]) {
		DeleteGraph(handle);
	}
	img[ID].clear();
}

//画像描画
//x,y　画面位置
//ID,num　オブジェクトIDと描画番号
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::DrawImg(float x, float y, int ID, int num, int TransFlag,int TurnY) {
	if (!IsValidImage(ID, num)) return;
	DrawRotaGraph2(x, y,0,0, 1, 0, img[ID][num], TransFlag, FALSE, TurnY);
}

//画像描画（0,0に描画）
//ID　オブジェクトID
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::DrawImg(int ID, int TransFlag) {
	if (!IsValidImage(ID, 0)) return;
	DrawGraph(0, 0, img[ID][0], TransFlag);
}

//画像描画
//x,y　画面位置
//ID,num　オブジェクトIDと描画番号
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::DrawImg(float x, float y, int ID, int TransFlag) {
	if (!IsValidImage(ID, 0)) return;
	DrawGraph(x, y, img[ID][0], TransFlag);
}


void ImageManager::DeleteAll() {
	InitGraph();
	for (auto& images : img) images.clear();
}
