#include "Object.h"
#include "DxLib.h"
#include "InputKey.h"
#include "Variable.h"
#include "ImageManager.h"
#include "PlayerManager.h"
#include "SoundManager.h"
#include "ObjectManager.h"
#include "GameData.h"
#include "Map.h"
#include "Conf.h"
#include "function.h"

//マップ座標との対応
int Object::LeftXMap() { return (int)((pos.x + gap.x) / (float)MAPSIZEX); }							//オブジェクトの左側をマップのX軸に対応させる
int Object::RightXMap() { return (int)((pos.x + (float)OBJSIZEX - gap.x - 1.) / (float)MAPSIZEX); }	//オブジェクトの右側をマップのX軸に対応させる
int Object::CenterXMap() { return (int)((pos.x + OBJSIZEX/2) / (float)MAPSIZEX); }							//オブジェクトの中央をマップのX軸に対応させる
int Object::UpYMap() { return (int)((pos.y + gap.y) / (float)MAPSIZEY); }							//オブジェクトの上側をマップのY軸に対応させる
int Object::BottomYMap() { return (int)((pos.y + (float)OBJSIZEY - 1.) / (float)MAPSIZEY); }			//オブジェクトの下側をマップのY軸に対応させる
int Object::CenterYMap() { return (int)((pos.y + OBJSIZEY/2) / (float)MAPSIZEY); }							//オブジェクトの中央をマップのY軸に対応させる

//１周り大きいマップ座標との対応
int Object::LeftXMapP1() { return (int)((pos.x + gap.x - 1.) / (float)MAPSIZEX); }							//オブジェクトの左側をマップのX軸に対応させる
int Object::RightXMapP1() { return (int)((pos.x + (float)OBJSIZEX - gap.x - 1. + 1.) / (float)MAPSIZEX); }	//オブジェクトの右側をマップのX軸に対応させる
int Object::UpYMapP1() { return (int)((pos.y + gap.y - 1) / (float)MAPSIZEY); }							//オブジェクトの上側をマップのY軸に対応させる
int Object::BottomYMapP1() { return (int)((pos.y + (float)OBJSIZEY - 1. + 1.) / (float)MAPSIZEY); }			//オブジェクトの下側をマップのY軸に対応させる

//マップとの当たり判定
int Object::MapHitRU(Map *M) {
	return M->GetNum(RightXMap(), UpYMap());		//オブジェクトの右上
}
int Object::MapHitLU(Map *M) {
	return M->GetNum(LeftXMap(), UpYMap());		//オブジェクトの左上
}
int Object::MapHitRB(Map *M) {
	return M->GetNum(RightXMap(), BottomYMap());	//オブジェクトの右下
}
int Object::MapHitLB(Map *M) {
	return M->GetNum(LeftXMap(), BottomYMap());	//オブジェクトの左下
}
int Object::MapHitCB(Map *M) {
	return M->GetNum(CenterXMap(), BottomYMap());	//オブジェクトの中央下
}
int Object::MapHitCU(Map* M) {
	return M->GetNum(CenterXMap(), UpYMap());	//オブジェクトの中央上
}
int Object::MapHitCC(Map* M) {
	return M->GetNum(CenterXMap(), CenterYMap());	//オブジェクトの中央上
}
int Object::MapHitRUP1(Map *M) {
	return M->GetNum(RightXMapP1(), UpYMapP1());		//オブジェクトの右側と上側
}
int Object::MapHitLUP1(Map *M) {
	return M->GetNum(LeftXMapP1(), UpYMapP1());		//オブジェクトの左側と上側
}
int Object::MapHitRBP1(Map *M) {
	return M->GetNum(RightXMapP1(), BottomYMapP1());	//オブジェクトの右側と下側
}
int Object::MapHitLBP1(Map *M) {
	return M->GetNum(LeftXMapP1(), BottomYMapP1());	//オブジェクトの左側と下側
}
int Object::MapHitCBP1(Map *M) {
	return M->GetNum(CenterXMap(), BottomYMapP1());	//オブジェクトの中央と下側
}
int Object::MapHitCUP1(Map* M) {
	return M->GetNum(CenterXMap(), UpYMapP1());	//オブジェクトの中央と下側
}

//マップとの当たり判定
int Object::MapKillRU(Map* M) {
	return M->GetKill(RightXMap(), UpYMap());		//オブジェクトの右上
}
int Object::MapKillLU(Map* M) {
	return M->GetKill(LeftXMap(), UpYMap());		//オブジェクトの左上
}
int Object::MapKillRB(Map* M) {
	return M->GetKill(RightXMap(), BottomYMap());	//オブジェクトの右下
}
int Object::MapKillLB(Map* M) {
	return M->GetKill(LeftXMap(), BottomYMap());	//オブジェクトの左下
}
int Object::MapKillCB(Map* M) {
	return M->GetKill(CenterXMap(), BottomYMap());	//オブジェクトの中央下
}
int Object::MapKillCU(Map* M) {
	return M->GetKill(CenterXMap(), UpYMap());	//オブジェクトの中央上
}

int Object::MapKillRUP1(Map* M) {
	return M->GetKill(RightXMapP1(), UpYMapP1());		//オブジェクトの右側と上側
}
int Object::MapKillLUP1(Map* M) {
	return M->GetKill(LeftXMapP1(), UpYMapP1());		//オブジェクトの左側と上側
}
int Object::MapKillRBP1(Map* M) {
	return M->GetKill(RightXMapP1(), BottomYMapP1());	//オブジェクトの右側と下側
}
int Object::MapKillLBP1(Map* M) {
	return M->GetKill(LeftXMapP1(), BottomYMapP1());	//オブジェクトの左側と下側
}
int Object::MapKillCBP1(Map* M) {
	return M->GetKill(CenterXMap(), BottomYMapP1());	//オブジェクトの中央と下側
}
int Object::MapKillCUP1(Map* M) {
	return M->GetKill(CenterXMap(), UpYMapP1());	//オブジェクトの中央と下側
}

void Object::HitBlock(Map *M){}
void Object::HitBlockB(Map *M) {}
void Object::HitBlockL(Map *M) {}
void Object::HitBlockR(Map *M) {}

void Object::TouchedBlock(Map *M) {}

void Object::RevisXR(Map *M) {
	HitBlockR(M);
	pos.x = ((int)(pos.x / (float)MAPSIZEX) + 1.) * MAPSIZEX - OBJSIZEX + gap.x;
}
void Object::RevisXL(Map *M) {
	HitBlockL(M);
	pos.x = ((int)(pos.x / (float)MAPSIZEX) + 1.) * MAPSIZEX - gap.x;
}
void Object::RevisYU(Map *M) {
	HitBlock(M);
	pos.y = (int)((pos.y / (float)MAPSIZEY) + 1.) * (float)MAPSIZEY;
}
void Object::RevisYB(Map *M) {
	HitBlockB(M);
	pos.y = (int)((pos.y + (float)OBJSIZEY + 1.) / (float)MAPSIZEY) * (float)MAPSIZEY - (float)OBJSIZEY;
}

int Object::BottomLBlockKill(Map *M) {
	return M->GetKill(LeftXMap(), BottomYMapP1());	//オブジェクトの左側と下側
}

int Object::BottomRBlockKill(Map *M) {
	return M->GetKill(RightXMap(), BottomYMapP1());	//オブジェクトの左側と下側
}

void Object::BlockKilled(Map *M) {
	if (BottomLBlockKill(M) == 2 || BottomLBlockKill(M) == 3 || BottomRBlockKill(M) == 2 || BottomRBlockKill(M) == 3) { GetDamage(0); }
	if (BottomLBlockKill(M) == 5 || BottomLBlockKill(M) == 6 || BottomRBlockKill(M) == 5 || BottomRBlockKill(M) == 6) { Killed(); }
}

int Object::CalcScore(int Num) {
	if (Num == 0)return 100;
	if (Num == 1)return 200;
	if (Num == 2)return 400;
	if (Num == 3)return 800;
	if (Num == 4)return 1000;
	if (Num == 5)return 2000;
	if (Num == 6)return 4000;
	if (Num == 7)return 5000;
	if (Num == 8)return 8000;
	PlayerManager::GetInstance().PlusZanki(1);
	SoundManager::GetInstance().PlaySE(EXTEND);
	return 0;
}

//オブジェクト生成
Object::Object(int x, int y) {
	Combo = 0;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	dire = -1;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = 0.0;
	gap.x = 8.0;
	gap.y = 0.0;
	HP = 1;
	dire = 1;
	alive = true;
	OnField = true;
	Tobiori = false;
}

Object::Object() {
	Combo = 0;
	OnField = true;
}

//X軸移動
void Object::MoveX(Map *M) {
	pos.x += speed.x;
	if (speed.x > 0.) {
		dire = 1;
		if (MapHitRB(M) == 1 || MapHitRU(M) == 1) {
			RevisXR(M);
			speed.x = 0;
		}
	}
	if (speed.x < 0.) {
		dire = -1;
		if (MapHitLB(M) == 1 || MapHitLU(M) 
			
			== 1) {
			RevisXL(M);
			speed.x = 0;
		}
	}
	if (pos.x + gap.x <= 0.) {
		pos.x = -gap.x;
		speed.x = 0;
	}
	if (pos.x + OBJSIZEX - gap.x >= M->GetWorldSize().x*MAPSIZEX) {
		pos.x = M->GetWorldSize().x*MAPSIZEX - OBJSIZEX + gap.x;
		speed.x = 0;
	}
}

//Y軸移動
void Object::MoveY(Map *M) {
	pos.y += speed.y;
	Water = (MapHitCC(M) == 5);
	if (MapHitLB(M) == 1 || MapHitRB(M) == 1) { Landing(M); }
	if (MapHitLB(M) == 11 || MapHitRB(M) == 11) {
		if ((int)(pos.y + (float)OBJSIZEY - 1.) % MAPSIZEY <= (int)abs(Vmax.y) && speed.y > 0.) { Landing(M); }
	}
	if (MapHitLB(M) == 12 || MapHitRB(M) == 12) {
		if ((int)(pos.y + (float)OBJSIZEY - 1.) % MAPSIZEY <= (int)abs(Vmax.y) && speed.y > 0.) { Landing(M); }
	}
	if (MapHitLU(M) == 1 || MapHitRU(M) == 1) {
		RevisYU(M);
		speed.y = 0.5;
	}
}

//着地処理
void Object::Landing(Map *M) {
	RevisYB(M);
	speed.y = 0.;
	Land = true;
	return;
}

//X軸速度変更
void Object::UpdateSpeedX() {}

//Y軸速度変更
void Object::UpdateSpeedY() {
	//重力
	if (!Water) {
		speed.y += GRAVITY;
		if (speed.y >= Vmax.y)speed.y = Vmax.y;
	} else {
		speed.y += GRAVITY / 3.;
		if (speed.y >= Vmax.y / 2.)speed.y = Vmax.y / 2.;
	}
	//崖から落ちた時（速度１以上）は着地してない判定にする
	if (speed.y >= 1. + GRAVITY)Land = false;
}

void Object::setX(float x) { pos.x = x; }
void Object::setY(float y) { pos.y = y; }
void Object::setHP(int x) { HP = x; }
float Object::getX() { return pos.x; }
float Object::getY() { return pos.y; }

bool Object::GetAlive() { return alive; }

void Object::Killed() {
	SoundManager::GetInstance().PlaySE(THREAD);
	alive = false;
}
void Object::GetDamage(int Num) {
	HP--;
	GameData::GetInstance().PlusScore(CalcScore(Num));
	ObjectManager::GetInstance().MakeScoreEffect(Num,getX(), getY() - 16);
	ObjectManager::GetInstance().MakeEffect(GetRand(5), getX(), getY());
	if (HP <= 0)Killed();
}
void Object::NaturalKilled(int Num) {
	GameData::GetInstance().PlusScore(CalcScore(Num));
	ObjectManager::GetInstance().MakeScoreEffect(Num,getX(), getY() - 16);
	ObjectManager::GetInstance().MakeEffect(GetRand(5), getX(), getY());
	alive = false;
}

void Object::Delete() { alive = false; }
void Object::PlusCombo() { Combo++; }
int Object::GetCombo() { return Combo; }

int Object::GetDire() {
	return dire;
}

void ItemObj::MoveY(Map *M) {
	pos.y += speed.y;
}
void ItemObj::animation() {
	image = time / 4;
	time++;
	if (time >= 16)time = 0;
}

void ItemObj::draw(Map *M) {
	ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), ITEM, BaseImg + image, TRUE);
}


bool Charactor::InScreen(Map *M) {
	float tmpX = M->GetSLU().x;
	float tmpY = M->GetSLU().y;
	if ((tmpX-MAPSIZEX*5.) < pos.x && (tmpX + WINDOWX + MAPSIZEX*5.) > pos.x ) {
			return true;
	}
	return false;
}

bool Charactor::InitInScreen(Map *M) {
	float tmpX = M->GetSLU().x;
	float tmpY = M->GetSLU().y;
	if ((tmpX - MAPSIZEX * 5.) < initpos.x && (tmpX + WINDOWX + MAPSIZEX * 5.) > initpos.x) {
		return true;
	}
	return false;
}

void Charactor::ReturnInitPos() {
	pos.x = initpos.x;
	pos.y = initpos.y;
}

void Charactor::update(Map *M) {
	UpdateSpeedX();
	MoveX(M);
	UpdateSpeedY();
	MoveY(M);
	TouchedBlock(M);
	animation();
}

void Charactor::draw(Map *M) {
	if(OnField)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), ENEMY, BaseImg + image, TRUE);
}