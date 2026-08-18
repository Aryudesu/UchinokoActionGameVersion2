#pragma once
#include "DxLib.h"

class Block {
protected:
	int animation;		//アニメーション用画像チップ番号
	int num;			//判定番号
	int Kill;			//攻撃判定の種類 : 0->判定なし 1->自機に判定あり 2->敵のみ攻撃判定あり 3->自機敵に判定あり 4->自機のみ即死 5->敵のみ即死 6->自機敵即死判定
	int img;			//背景画像的なやつ
	int BlImg;			//ベースとなる画像番号
	bool alive;			//存在判定
	bool anime;			//アニメーションするかしないか
	VECTOR pos;
	int time;			//アニメーション用タイム

public:
	Block();

	virtual void SetImg(int n = 0);
	int GetImg();

	virtual void update() = 0;

	void SetPos(float x, float y);

	//描画関数
	virtual void draw(float x, float y, float sx, float sy);
	virtual void BackDraw(float x, float y, float sx, float sy);
	virtual void BlDraw(float x, float y, float sx, float sy);
	virtual void Touched();
	virtual void Hited();			//下から叩かれた時
	virtual void On();				//上に乗られた時
	virtual void PushL();			//左から押された時
	virtual void PushR();			//右から押された時
	void SetKill(int Num);

	int GetNum();
	int GetKill();
};

//存在しないブロック
class Empty : public Block {
public:
	Empty();
	void BlDraw(float x, float y, float sx, float sy);//何もしない
	void update();
};

//透明ブロック
class Steals : public Block {
public:
	Steals();
	void BlDraw(float x, float y, float sx, float sy);//何もしない
	void update();
};

//レンガブロック
class Bricks : public Block {
public:
	Bricks();
	void update();
	void Hited();
};

//コイン
class Coin : public Block {
public:
	Coin();
	void update();
	void Touched();
};
//回復コイン
class HealingCoin : public Block {
public:
	HealingCoin();
	void update();
	void Touched();
};
//1UPコイン
class OneUPCoin : public Block {
public:
	OneUPCoin();
	void update();
	void Touched();
};

//？ブロック
class ItemBlock : public Block {
protected:
	bool Brown;
public:
	ItemBlock();
	void update();
	void Hited();
	virtual void MakeItem() = 0;
};

class CoinBlock : public ItemBlock {
public:
	CoinBlock();
	void MakeItem();
};

class Coin10Block : public ItemBlock {
	int count;
public:
	Coin10Block();
	void update();
	void Hited();
	void MakeItem();
};

//茶色ブロック
class BrownBlock : public Block {
public:
	BrownBlock();
	void update();
};

class HealingBlock : public ItemBlock {
public:
	HealingBlock();
	void MakeItem();
};

class OneUpBlock : public ItemBlock {
public:
	OneUpBlock();
	void MakeItem();
};

class LadderMakerBlock : public ItemBlock {
public:
	LadderMakerBlock();
	void MakeItem();
};


//透明？ブロック
class StealItemBlock : public Block {
protected:
	bool Brown;
public:
	StealItemBlock();
	void update();
	void Hited();
	virtual void MakeItem() = 0;
};

class StealCoinBlock : public StealItemBlock {
public:
	StealCoinBlock();
	void MakeItem();
};

class StealHealingBlock : public StealItemBlock {
public:
	StealHealingBlock();
	void MakeItem();
};

class StealOneUpBlock : public StealItemBlock {
public:
	StealOneUpBlock();
	void MakeItem();
};

class StealLadderMakerBlock : public StealItemBlock {
public:
	StealLadderMakerBlock();
	void MakeItem();
};


//はしご
class Ladder : public Block {
public:
	Ladder();
	void update();
};

//下から登れる
class Cloud : public Block {
public:
	Cloud();
	void update();
};

//降りれる
class Through : public Block {
public:
	Through();
	void update();
};

//土管
class PipeUL : public Block {
public:
	PipeUL();
	void update();
};

//土管
class PipeUR : public Block {
public:
	PipeUR();
	void update();
};

//土管
class PipeDL : public Block {
public:
	PipeDL();
	void update();
};

//土管
class PipeDR : public Block {
public:
	PipeDR();
	void update();
};


//土管
class PipeLU : public Block {
public:
	PipeLU();
	void update();
};

//土管
class PipeLD : public Block {
public:
	PipeLD();
	void update();
};

//土管
class PipeRU : public Block {
public:
	PipeRU();
	void update();
};

//土管
class PipeRD : public Block {
public:
	PipeRD();
	void update();
};

//クリアアイテム
class BeatItem :public Block {
public:
	BeatItem();
	void update();
	void Touched();
};

//クリアアイテム２
class BeatItem2 :public Block {
public:
	BeatItem2();
	void update();
	void Touched();
};

class SakaDRU : public Block {
public:
	SakaDRU();
	void update();
};

//コイン0枚のときに通れるブロック
class ZeroCoinBlock : public Block {
public:
	ZeroCoinBlock();
	void update();
};

//コイン50枚以上のときに通れるブロック
class O50CoinBlock : public Block {
public:
	O50CoinBlock();
	void update();
};

//コイン50枚未満のときに通れるブロック
class U50CoinBlock : public Block {
public:
	U50CoinBlock();
	void update();
};

//消えたり現れたりするブロック
class DisAppBlock1 : public Block {
public:
	DisAppBlock1();
	void update();
};

class DisAppBlock2 : public Block {
public:
	DisAppBlock2();
	void update();
};

//ONOFFで消えたり現れたりするブロック
class ONOFFDABlock1 : public Block {
public:
	ONOFFDABlock1();
	void update();
};
class ONOFFDABlock2 : public Block {
public:
	ONOFFDABlock2();
	void update();
};

//ON-OFFブロック
class ONOFF : public Block {
public:
	ONOFF();
	void update();
	void Hited();
};

//水中ブロック
class WaterBlock : public Block {
public:
	WaterBlock();
	void update();
};

//上方向重力ブロック
class UpGravBlock : public Block {
public:
	UpGravBlock();
	void update();
};
//下方向重力ブロック
class DownGravBlock : public Block {
public:
	DownGravBlock();
	void update();
};
