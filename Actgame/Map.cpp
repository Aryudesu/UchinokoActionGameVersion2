#include "Map.h"
#include "Block.h"
#include "BlockFactory.h"
#include "ObjectManager.h"
#include "DxLib.h"
#include "function.h"
#include "PlayerManager.h"
#include "Conf.h"
#include <vector>
#include <string>

//マップサイズ取得
void Map::SetSize(std::vector<std::vector <int>> Dat) {
	for (int i = 0; i < Dat.size(); i++) {
		if (Size.x < Dat[i].size())Size.x = Dat[i].size();
	}
	Size.y = Dat.size();
	dat.resize(Size.x);
	for (int i = 0; i < Size.x; i++)dat[i].resize(Size.y);
}

//マップ読み込み
void Map::LoadMap(std::string FileName) {
	std::vector<std::vector <int>> tmp = LoadArray(FileName);
	SetSize(tmp);
	BlockFactory BF;
	for (int j = 0; j < Size.y; j++) {
		for (int i = 0; i < Size.x; i++) {
			if (i >= tmp[j].size() || j >= tmp.size()) {
				dat[i][j] = BF.BlkFactory(0);
				dat[i][j]->SetPos(i * 32, j * 32);
				continue;
			}
			if (tmp[j][i] >= 0 && tmp[j][i] <= 50) {
				dat[i][j] = BF.BlkFactory(tmp[j][i]);
			} else {
				dat[i][j] = BF.BlkFactory(0);
				if (tmp[j][i] == -1)PlayerManager::GetInstance().SetInitPos(i * 32, j * 32);
				if (tmp[j][i] == -2)ObjectManager::GetInstance().MakeEnemy(1, i * 32, j * 32);
				if (tmp[j][i] == -3)ObjectManager::GetInstance().MakeEnemy(2, i * 32, j * 32);
				if (tmp[j][i] == -4)ObjectManager::GetInstance().MakeEnemy(3, i * 32, j * 32);
				if (tmp[j][i] == -5)ObjectManager::GetInstance().MakeEnemy(4, i * 32, j * 32);
				if (tmp[j][i] == -6)ObjectManager::GetInstance().MakeEnemy(5, i * 32, j * 32);
			}
			dat[i][j]->SetPos(i * 32, j * 32);
		}
	}
}

//マップの画像チップデータ読み込み
void Map::SetMapChip(std::string FileName) {
	std::vector<std::vector <int>> tmp;
	tmp = LoadArray(FileName);
	BlockFactory BF;
	for (int j = 0; j < Size.y; j++) {
		for (int i = 0; i < Size.x; i++) {
			if (tmp[j][i] >= 0) {
				dat[i][j]->SetImg(tmp[j][i]);
			}
		}
	}
}


void Map::SetScreenLU(float x, float y) {
	if (ScrollMode == 0) {
		if (x + OBJSIZEX / 2 > WINDOWX / 2) { ScreenLU.x = (int)(x - WINDOWX / 2. + OBJSIZEX / 2.); }
		else { ScreenLU.x = 0; }
		if (x + OBJSIZEX / 2 > Size.x * MAPSIZEX - WINDOWX / 2) { ScreenLU.x = (int)(Size.x * MAPSIZEX - WINDOWX); }
		if (y + OBJSIZEY / 2 > WINDOWY / 2) { ScreenLU.y = (int)((y - WINDOWY / 2. + OBJSIZEY / 2.)); }
		else { ScreenLU.y = 0; }
		if (y + OBJSIZEY / 2 > Size.y * MAPSIZEY - WINDOWY / 2) { ScreenLU.y = (int)(Size.y * MAPSIZEY - WINDOWY); }
		if (ScreenLU.x < 0)ScreenLU.x = 0;
		if (ScreenLU.y < 0)ScreenLU.y = 0;
		return;
	}
	if (ScrollMode == 1 || ScrollMode == 2 || ScrollMode == 3) {
		ScreenLU.x += ScrollMode;
		if (ScreenLU.x < 0)ScreenLU.x = 0;
		if (ScreenLU.x + WINDOWX > MAPSIZEX * Size.x)ScreenLU.x = MAPSIZEX * Size.x - WINDOWX;
		if (y + OBJSIZEY / 2 > WINDOWY / 2) { ScreenLU.y = (int)((y - WINDOWY / 2. + OBJSIZEY / 2.)); }
		else { ScreenLU.y = 0; }
		if (y + OBJSIZEY / 2 > Size.y * MAPSIZEY - WINDOWY / 2) { ScreenLU.y = (int)(Size.y * MAPSIZEY - WINDOWY); }
		return;
	}
	if (ScrollMode == 4) {
		ScreenLU.x = 0;
		ScreenLU.y = 0;
		return;
	}
	if (ScrollMode == 5) {
		ScreenLU.y = 0;
		if (x + OBJSIZEX / 2 > WINDOWX / 2) { ScreenLU.x = (int)(x - WINDOWX / 2. + OBJSIZEX / 2.); }
		else { ScreenLU.x = 0; }
		if (x + OBJSIZEX / 2 > Size.x * MAPSIZEX - WINDOWX / 2) { ScreenLU.x = (int)(Size.x * MAPSIZEX - WINDOWX); }
		if (ScreenLU.x < 0)ScreenLU.x = 0;
	}
}

float Map::GetScreenLUX() { return ScreenLU.x; }
float Map::GetScreenLUY() { return ScreenLU.y; }

Map::Map(std::string FileName, std::string FileName2) {
	LoadMap(FileName);
	SetMapChip(FileName2);
	ScrollMode = 0;
}

//マップ更新
void Map::update() {
	for (int j = 0; j < Size.y; j++) {
		for (int i = 0; i < Size.x; i++) {
			dat[i][j]->update();
		}
	}
}

//描画
void Map::draw() {
	for (int j = 0; j < Size.y; j++) {
		for (int i = 0; i < Size.x; i++)dat[i][j]->draw((float)i, (float)j, ScreenLU.x, ScreenLU.y);
	}
}

float Map::GetSX() {
	return SX;
}
float Map::GetSY() {
	return SY;
}

//広さを知りたい
VECTOR Map::GetWorldSize() { return Size; }

//ブロック番号
int Map::GetNum(int x, int y) {
	int tmpx = x;
	int tmpy = y;
	if (tmpx < 0)tmpx = 0;
	if (tmpx >= Size.x)tmpx = Size.x - 1;
	if (tmpy < 0)tmpy = 0;
	if (tmpy >= Size.y)tmpy = Size.y - 1;
	return dat[tmpx][tmpy]->GetNum();
}

int Map::GetKill(int x, int y) {
	int tmpx = x;
	int tmpy = y;
	if (tmpx < 0)tmpx = 0;
	if (tmpx >= Size.x)tmpx = Size.x - 1;
	if (tmpy < 0)tmpy = 0;
	if (tmpy >= Size.y)tmpy = Size.y - 1;
	return dat[tmpx][tmpy]->GetKill();
}

void Map::Hited(int x, int y) {
	if (x < 0)return;
	if (x >= Size.x)return;
	if (y < 0)return;
	if (y >= Size.y)return;
	dat[x][y]->Hited();
}

void Map::Touched(int x, int y) {
	if (x < 0)return;
	if (x >= Size.x)return;
	if (y < 0)return;
	if (y >= Size.y)return;
	dat[x][y]->Touched();
}

void Map::SetBlock(int num, int x, int y) {
	BlockFactory BF;
	int tmp = dat[x][y]->GetImg();
	delete dat[x][y];
	dat[x][y] = BF.BlkFactory(num);
	dat[x][y]->SetPos(x * 32, y * 32);
	dat[x][y]->SetImg(tmp);
}

VECTOR Map::GetSLU() { return ScreenLU; }
void Map::SetScrollMode(int Num) { ScrollMode = Num; }
int Map::GetScrollMode() { return ScrollMode; }

void Map::DeleteAll() {
	for (int i = 0; i < dat.size(); i++) {
		for (int j = 0; j < dat[i].size(); j++)delete dat[i][j];
		dat[i].clear();
	}
	dat.clear();
}

Map::~Map() {
	DeleteAll();
}