#pragma once
#include "Object.h"

class Player : public Charactor {
private:
	int ImgCount;
	int DamageTime;

	int Moving;
	int MovingTime;
	int GravDire = 1;			//重力発生方向

	bool Dead;
	int DeadNum = 0;

	bool Beat;

	float ACCE = 0.13;
	float D2WA = 1.2;
	float DASH = 1.3;

	int JumpCount;					//ジャンプ回数カウント用（２段ジャンプとかの多段ジャンプを考えるためのもの）
	bool JumpFlag;				//ジャンプボタン押した判定用
	bool Damage;				//ダメージモーション
	int  DamageNum;
	bool Ladd;				//ハシゴつかまってる時

	void HitBlock(Map *M);		//ブロック叩いた
	void HitBlockB(Map* M);		//ブロック叩いた
	void TouchedBlock(Map *M);	//ブロックに触れた

	bool Lad(Map *M);			//ハシゴに触れた時
	void LadderAct(Map *M);		//ハシゴアクション時

	void DamageUpdate();	//ダメージ受けた時に実行される関数

	void UpdateSpeedX();	//X軸速度更新
	void UpdateSpeedY();	//Y軸速度更新
	void MoveX(Map *M);
	void MoveY(Map *M);
	void Jump();			//ジャンプ関数
	void animation();		//アニメーション関数

	void DeadMotion(Map *M);
	void DamageMotion(Map *M);
	void LadderMotion(Map *M);
	void NormalMotion(Map *M);
public:
	Player();

	void SetInitPos(float x, float y);
	void Init(float x, float y);

	void draw(Map *M);
	void update(Map *M);	//いろいろ更新

	void Emerge(int Num);

	int GetHP();			//HPを取得
	void PlusHP(int n);		//HPに足す
	void SetHP(int n);		//HPをセット

	void ReturnInitPos();	//初期位置に戻す

	void Tread(int n = 1);		//敵を踏んだ時	1:通常

	void Damaged(int direction,int n = 1);	//ダメージ受けた時	1:通常

	int GetDeadNum();
	void Death(int Num = 0);

	void Move(Map *M);
	int GetMove();
	int GetMoveTime();

	bool GetDead();
	int GetGravDire();

	void SetBeat(bool Param);
	bool GetBeat();
};