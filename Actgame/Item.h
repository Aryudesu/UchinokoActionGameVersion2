#pragma once
#include "Object.h"

class Map;

class CoinItem : public ItemObj {
private:
public:
	CoinItem(int x,int y);
	void update(Map *M);
};

class HealingItem : public ItemObj {
private:
public:
	HealingItem(int x, int y);
	void update(Map *M);
};

class OneUpItem : public ItemObj {
private:
public:
	OneUpItem(int x, int y);
	void update(Map *M);
};