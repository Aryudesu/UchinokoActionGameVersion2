#pragma once
#include "Map.h"

//オブジェクト全体
class Object {
protected:
	VECTOR initpos;
	VECTOR pos;
	VECTOR speed;
	VECTOR Vmax;
	VECTOR gap;

	int Combo;
	int dire;
	int image;
	int HP;
	int time;
	bool OnField;		//フィールド上に存在するかどうか
	bool alive;			//trueだと存続　falseだと消去する
	bool Land;			//着地判定
	bool Tobiori;		//降りれる床から飛び降りる時の判定
	bool OnSaka;		//坂道着地判定
	bool Water;			//水中

	//ゲーム座標からマップ座標に変換する
	int LeftXMap();		//オブジェクトの左側をマップのX軸に対応させる
	int RightXMap();		//オブジェクトの右側をマップのX軸に対応させる
	int CenterXMap();		//オブジェクトの中央をマップのX軸に対応させる
	int UpYMap();			//オブジェクトの上側をマップのX軸に対応させる
	int BottomYMap();	//オブジェクトの下側をマップのX軸に対応させる
	int CenterYMap();	//オブジェクトの下側をマップのX軸に対応させる

	int LeftXMapP1();		//オブジェクトの左側をマップのX軸に対応させる
	int RightXMapP1();		//オブジェクトの右側をマップのX軸に対応させる
	int UpYMapP1();			//オブジェクトの上側をマップのX軸に対応させる
	int BottomYMapP1();	//オブジェクトの下側をマップのX軸に対応させる

	//マップとの当たり判定
	int MapHitRU(Map *M);	//右上
	int MapHitLU(Map *M);	//左上
	int MapHitRB(Map *M);	//右下
	int MapHitLB(Map *M);	//左下
	int MapHitCB(Map *M);	//中下
	int MapHitCU(Map* M);	//中上
	int MapHitCC(Map* M);	//中上
	int MapHitRUP1(Map *M);	//右上
	int MapHitLUP1(Map *M);	//左上
	int MapHitRBP1(Map *M);	//右下
	int MapHitLBP1(Map *M);	//左下
	int MapHitCBP1(Map *M);	//中下
	int MapHitCUP1(Map* M);	//中上
	//接してるブロックのダメージ番号
	int MapKillRU(Map* M);	//右上
	int MapKillLU(Map* M);	//左上
	int MapKillRB(Map* M);	//右下
	int MapKillLB(Map* M);	//左下
	int MapKillCB(Map* M);	//中下
	int MapKillCU(Map* M);	//中上
	int MapKillRUP1(Map* M);	//右上
	int MapKillLUP1(Map* M);	//左上
	int MapKillRBP1(Map* M);	//右下
	int MapKillLBP1(Map* M);	//左下
	int MapKillCBP1(Map* M);	//中下
	int MapKillCUP1(Map* M);	//中上
	//判定あるブロックにあたった時の座標補正
	virtual void RevisXR(Map *M);
	virtual void RevisXL(Map *M);
	virtual void RevisYU(Map *M);
	virtual void RevisYB(Map *M);

	//着地処理
	virtual void Landing(Map *M);
	//virtual bool OnBlock(int Num);

	virtual void HitBlock(Map *M);
	virtual void HitBlockB(Map *M);
	virtual void HitBlockL(Map *M);
	virtual void HitBlockR(Map *M);

	virtual void TouchedBlock(Map *M);
	virtual int BottomLBlockKill(Map *M);
	virtual int BottomRBlockKill(Map *M);
	virtual void BlockKilled(Map *M);

public:
	//オブジェクト生成
	Object(int x, int y);
	Object();

	//速度更新
	virtual void UpdateSpeedX();
	virtual void UpdateSpeedY();

	virtual void MoveX(Map *M);		//X軸移動
	virtual void MoveY(Map *M);		//Y軸移動

	virtual void update(Map *M) = 0;
	virtual void animation() = 0;
	virtual void draw(Map *M) = 0;
	
	void setX(float x);
	void setY(float y);
	void setHP(int x);

	float getX();
	float getY();
	float getgapX() { return gap.x; }
	float getgapY() { return gap.y; }

	bool GetAlive();
	bool GetOnField() { return OnField; }

	virtual void GetDamage(int Num);
	virtual void Killed();
	void NaturalKilled(int Num);
	void Delete();

	virtual int CalcScore(int Num);

	virtual ~Object() {}

	void PlusCombo();
	int GetCombo();

	int GetDire();
};

//他キャラ
class Charactor : public Object {
protected:
	//踏んでも大丈夫な敵かどうか
	bool Tread_;

	//基本画像番号
	int BaseImg;

	//HP
	//int HP;

	int PDecision;		//主人公への判定	0:なし	1:攻撃判定あり	2:攻撃判定は無いが当たり判定はある
	int EDecision;		//敵同士の判定		0:なし	1:攻撃判定あり	2:攻撃判定は無いが当たり判定はある
	int Press;			//1:踏める 2:踏めない 0:踏む時の当たり判定がない
public:
	bool GetTreadAble() { return Tread_; }				//踏めるかどうか返すだけ
	virtual void Treaded() {}							//踏まれた時
	void update(Map *M);								//更新
	virtual void animation() = 0;						//アニメーション
	void draw(Map *M);									//描画
	bool InScreen(Map *M);								//画面内かどうか
	bool InitInScreen(Map *M);							//初期位置が画面内かどうか
	void ReturnInitPos();								//初期位置に戻す
	virtual void ColliSide2Obj(float x, float y) {}		//他のオブジェクトにぶつかった時の処理
	virtual void ColliSide2ObjP(float x, float y) {}	//プレイヤーにぶつかった時の処理

	int GetPress() { return Press; }
	int GetEDecision() { return EDecision; }
	int GetPDecision() { return PDecision; }
};


//アイテム
class ItemObj : public Object {
protected:
	int BaseImg;
public:
	void MoveY(Map *M);
	void update(Map *M) {};
	void animation();
	void draw(Map *M);
};

class Effect : public Object {
protected:
	int Count;
	int BaseImg;
public:
	Effect();
	Effect(int ID,int x, int y);
	void animation() {};
	void update(Map *M);
	void draw(Map *M);
};

class ScoreEffect :public Effect {
public:
	ScoreEffect();
	ScoreEffect(int ID,int x, int y);
	void update(Map *M);
	void draw(Map *M);
};

class BlockFragment :public Effect {
public:
	BlockFragment();
	BlockFragment(int ID, int x, int y);
	void update(Map* M);
	void draw(Map* M);
};

class EffectObj : public Effect {
public:
	EffectObj();
	EffectObj(int ID, int x, int y);
	void update(Map *M);
	void draw(Map* M);
};