#pragma once
#include "Object.h"

//歩く敵テンプレート
class WalkingEnemy : public Charactor {
public:
	WalkingEnemy(int x, int y);
	WalkingEnemy() {};
	virtual void animation() {};
	virtual void update(Map *M);
	virtual void UpdateSpeedX();
	virtual void MoveX(Map *M);
	void ColliSide2Obj(float x, float y);
	void ColliSide2ObjP(float x, float y);
	virtual void Treaded();
};

//通常版歩く敵
class WalkingEnemy1 : public WalkingEnemy{
public:
	WalkingEnemy1(int x, int y);
	void animation();
};

//崖に直面すると引き返す敵
class WalkingEnemy2 : public WalkingEnemy {
public:
	WalkingEnemy2(int x, int y);
	void animation();
	void MoveX(Map *M);
};

//地面から出てくる敵
class CarrotMan : public WalkingEnemy {
public:
	CarrotMan(int x, int y);
	CarrotMan() {};
	void animation();
	void update(Map *M);
	void UpdateSpeedX();
	void Landing(Map *M);
};

//地面から出てくる亀（崖から落ちる）
class BallSlime : public WalkingEnemy {
protected:
	int Mode;
	int DecTime;
	int BKTime;
	const int JumpTime = 5;
	const int FukkatsuTime = 7;
public:
	BallSlime(int x, int y);
	BallSlime() {};
	void update(Map *M);
	void animation();
	void UpdateSpeedX();
	void ColliSide2Obj(float x, float y);
	void ColliSide2ObjP(float x, float y);
	void Treaded();
	void BlockKilled(Map *M);

	void HitBlock(Map *M);
	void HitBlockL(Map *M);
	void HitBlockR(Map *M);
};

class BallSlime2 : public BallSlime {
public:
	BallSlime2(int x, int y);
	BallSlime2() {};
	void MoveX(Map *M);
};