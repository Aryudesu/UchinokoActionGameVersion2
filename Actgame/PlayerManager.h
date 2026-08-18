#pragma once
#include "Player.h"
#include "Singleton.h"
#include <memory>
#include <vector>
class Map;

class PlayerManager : public Singleton <PlayerManager> {
private:
	std::unique_ptr<Player> Ply;
	int Coin;
	int HP = 3;
	std::vector<int> BeatLevel;
public:
	//設置
	void SetPlayer(float x, float y);
	//初期位置設定
	void SetInitPos(float x, float y);
	//初期位置に戻る
	void ReturnInitPos();

	//初期化
	void InitPlayer();

	//座標取得
	float GetPlayerX();
	float GetPlayerY();

	float GetGapX() { return Ply->getgapX(); }
	float GetGapY() { return Ply->getgapY(); }

	void update(Map *M);
	void draw(Map *M);

	//HP
	int GetHP();
	void PlusHP(int n);

	//コイン
	int GetCoin();
	void PlusCoin(int n);
	void SetCoin(int n);


	void SetHP(int n);
	//残機
	int GetZanki();			//今の残機取得
	void PlusZanki(int n);	//残機増やす

	void Damaged(int direction, int n = 1);

	void Tread(int n = 1) { Ply->Tread(n); }

	void Death(int Num = 1);

	void Init(float x, float y);

	void Move(Map *M);
	int GetMove();
	int GetMoveTime();

	void Emerge(int Num);

	bool GetDead();
	int GetDeadNum();

	void ResetHP();

	void SetBeat(bool Param);
	bool GetBeat();

	int GetCombo();
	void PlusCombo();
	int GetDire();
	int GetGravDire();
};
