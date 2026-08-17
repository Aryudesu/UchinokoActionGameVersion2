#include "PlayerManager.h"
#include "SoundManager.h"
#include "Player.h"
#include "Variable.h"
#include "GameData.h"
#include "ObjectManager.h"


void PlayerManager::SetPlayer(float x, float y) {
	Ply->setX(x);
	Ply->setY(y);
}

void PlayerManager::InitPlayer() {
	if (Ply != nullptr)delete Ply;
	Ply = new Player();
	Ply->SetHP(HP);
}

void PlayerManager::SetInitPos(float x,float y) {
	Ply->SetInitPos(x, y);
}

void PlayerManager::ReturnInitPos() {
	Ply->ReturnInitPos();
}

float PlayerManager::GetPlayerX() {
	return Ply->getX();
}

float PlayerManager::GetPlayerY() {
	return Ply->getY();
}


void PlayerManager::update(Map *M) {
	Ply->update(M);
}

void PlayerManager::draw(Map *M) {
	Ply->draw(M);
}

int PlayerManager::GetHP() {
	return HP;
}

void PlayerManager::SetHP(int n) {
	HP = n;
	if (HP <= 0)Death();
}

void PlayerManager::PlusHP(int n) {
	HP += n;
	if (HP <= 0)Death();
	if (HP > HPMAX)HP = HPMAX;
}

int PlayerManager::GetCoin() {
	return Coin;
}

void PlayerManager::PlusCoin(int n) {
	Coin += n;
	if (Coin >= 100) {
		PlusZanki(1);
		ObjectManager::GetInstance().MakeScoreEffect(9, GetPlayerX(), GetPlayerY() - 16);
		SoundManager::GetInstance().PlaySE(EXTEND);
		Coin -= 100;
	}
}
void PlayerManager::SetCoin(int n) { Coin = n; }

int PlayerManager::GetZanki() {
	return GameData::GetInstance().GetZanki();
}

void PlayerManager::PlusZanki(int n) {
	GameData::GetInstance().PlusZanki(n);
}

void PlayerManager::Damaged(int direction,int n) {
	Ply->Damaged(direction,n);
}

void PlayerManager::Death(int Num){
	Ply->Death(Num);
}

int PlayerManager::GetMove() { return Ply->GetMove(); }
int PlayerManager::GetMoveTime() { return Ply->GetMoveTime(); }

void PlayerManager::Init(float x, float y) {
	Ply->Init(x, y);
}

void PlayerManager::Emerge(int Num) {
	Ply->Emerge(Num);
}

bool PlayerManager::GetDead() {
	return Ply->GetDead();
}

void PlayerManager::Move(Map *M) { Ply->Move(M); }

int PlayerManager::GetDeadNum() { return Ply->GetDeadNum(); }

void PlayerManager::ResetHP() { HP = 3; }

void PlayerManager::SetBeat(bool Param) { Ply->SetBeat(Param); }
bool PlayerManager::GetBeat() { return Ply->GetBeat(); }

int PlayerManager::GetCombo() { return Ply->GetCombo(); }

void PlayerManager::PlusCombo() { Ply->PlusCombo(); }
int PlayerManager::GetDire() { return Ply->GetDire(); }

int PlayerManager::GetGravDire() { return Ply->GetGravDire(); }