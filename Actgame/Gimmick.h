#pragma once
#include "Object.h"

class LadderMaker : public Object {
public:
	LadderMaker(int x, int y);
	void UpdateSpeedX();	//速さ
	void UpdateSpeedY();	//速さ
	void MoveX(Map *M);		//X軸移動
	void MoveY(Map *M);		//Y軸移動
	void update(Map *M);	//更新

	void animation();
	void draw(Map *M);
};