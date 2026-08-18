#include "Item.h"
#include "Map.h"
#include "function.h"
#include "ImageManager.h"
#include "PlayerManager.h"
#include "GameData.h"
#include "ObjectManager.h"

//コイン
CoinItem::CoinItem(int x,int y) {
	BaseImg = 0;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	image = 0;
	time = 0;
	speed.x = 0;
	speed.y = -10;
	alive = true;
	Water = false;
}
void CoinItem::update(Map *M) {
	UpdateSpeedY();
	if (speed.y >= 5) {
		PlayerManager::GetInstance().PlusCoin(1);
		GameData::GetInstance().PlusScore(100);
		alive = false;
	}
	MoveY(M);
	animation();
}

//回復アイテム
HealingItem::HealingItem(int x, int y) {
	BaseImg = 4;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	image = 0;
	time = 0;
	speed.x = 0;
	speed.y = -10;
	alive = true;
	Water = false;
}
void HealingItem::update(Map *M) {
	UpdateSpeedY();
	if (speed.y >= 5) {
		PlayerManager::GetInstance().PlusHP(1);
		GameData::GetInstance().PlusScore(1000);
		ObjectManager::GetInstance().MakeScoreEffect(4, getX(), getY() - 16);
		alive = false;
	}
	MoveY(M);
	animation();
}

//1UPアイテム
OneUpItem::OneUpItem(int x, int y) {
	BaseImg = 8;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	image = 0;
	time = 0;
	speed.x = 0;
	speed.y = -10;
	alive = true;
	Water = false;
}
void OneUpItem::update(Map *M) {
	UpdateSpeedY();
	if (speed.y >= 5) {
		GameData::GetInstance().PlusZanki(1);
		ObjectManager::GetInstance().MakeScoreEffect(9, getX(), getY() - 16);
		alive = false;
	}
	MoveY(M);
	animation();
}