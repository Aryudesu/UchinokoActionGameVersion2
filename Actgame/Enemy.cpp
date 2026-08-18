#include "Enemy.h"
#include "PlayerManager.h"
#include "SoundManager.h"
#include "GameData.h"
#include "ObjectManager.h"
#include "Conf.h"

WalkingEnemy::WalkingEnemy(int x, int y) {
	gap.y = 1;
	Tread_ = true;
	Press = 1;
	PDecision = 1;
	EDecision = 2;
	HP = 1;
}

void WalkingEnemy::update(Map *M) {
	//画面内に存在してゲーム上に存在する場合
	if (InScreen(M) && OnField) {
		UpdateSpeedX();
		MoveX(M);
		UpdateSpeedY();
		MoveY(M);
		BlockKilled(M);
		animation();
	} else {
		//画面外にいる場合は一旦消去
		OnField = false;
	}
	//一旦消去されて、初期位置が画面外にある場合
	if (!OnField && !InitInScreen(M)) {
		ReturnInitPos();		//元の位置に戻してやる
		OnField = true;			//存在を戻してやる
		if (PlayerManager::GetInstance().GetPlayerX() > initpos.x)dire = 1;	//主人公が初期位置より右側にいるなら右側を向かせる
		else dire = -1;														//そうでなければ左側を向かせる
	}
}

void WalkingEnemy::UpdateSpeedX() { speed.x = (float)dire*2.0; }

void WalkingEnemy::ColliSide2Obj(float x, float y) {
	if (x > pos.x && dire == 1)dire = -1;
	if (x <= pos.x && dire == -1)dire = 1;
}
void WalkingEnemy::ColliSide2ObjP(float x, float y) {
	if (x > pos.x && dire == 1)dire = -1;
	if (x <= pos.x && dire == -1)dire = 1;
}

//X軸移動
void WalkingEnemy::MoveX(Map *M) {
		pos.x += speed.x;
		if (speed.x > 0.) {
			if (MapHitRB(M) == 1 || MapHitRU(M) == 1) {
				if (MapKillRB(M) == 2 || MapKillRB(M) == 3 || MapKillRU(M) == 2 || MapKillRU(M) == 3) GetDamage(0);
				if (MapKillRB(M) == 5 || MapKillRB(M) == 6 || MapKillRU(M) == 5 || MapKillRU(M) == 6) Killed();
				RevisXR(M);
				dire *= -1;
			}
		}
		if (speed.x < 0.) {
			dire = -1;
			if (MapHitLB(M) == 1 || MapHitLU(M)== 1) {
				if (MapKillLB(M) == 2 || MapKillLB(M) == 3 || MapKillLU(M) == 2 || MapKillLU(M) == 3) GetDamage(0);
				if (MapKillLB(M) == 5 || MapKillLB(M) == 6 || MapKillLU(M) == 5 || MapKillLU(M) == 6) Killed();
				RevisXL(M);
				dire *= -1;
			}
		}
		if (pos.x + gap.x <= 0.) {
			pos.x = -gap.x;
			dire *= -1;
		}
		if (pos.x + OBJSIZEX - gap.x >= M->GetWorldSize().x*MAPSIZEX) {
			pos.x = M->GetWorldSize().x*MAPSIZEX - OBJSIZEX + gap.x;
			dire *= -1;
		}
}

//主人国に踏まれた時
void WalkingEnemy::Treaded() {
	GetDamage(PlayerManager::GetInstance().GetCombo());
}

WalkingEnemy1::WalkingEnemy1(int x, int y) {
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = 0.0;
	gap.x = 8.0;
	gap.y = 1.0;
	time = 0;
	image = 0;
	BaseImg = 12*0;
	HP = 1;
	dire = -1;
	alive = true;
	Press = 1;
	PDecision = 1;
	EDecision = 2;
}

void WalkingEnemy1::animation() {
	if(dire == -1)image = time / 12;
	if(dire ==  1)image = time / 12 + 4;
	time++;
	if (time >= 12*4){
		time = 0;
		if(dire == -1)image = 0;
		else image = 4;
	}
}


WalkingEnemy2::WalkingEnemy2(int x, int y) {
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = 0.0;
	gap.x = 8.0;
	gap.y = 1.0;
	time = 0;
	image = 0;
	BaseImg = 12*1;
	HP = 1;
	dire = -1;
	alive = true;
	Press = 1;
	PDecision = 1;
	EDecision = 2;
}

void WalkingEnemy2::animation() {
	if (dire == -1)image = time / 12;
	if (dire == 1)image = time / 12 + 4;
	time++;
	if (time >= 12 * 4) {
		time = 0;
		if (dire == -1)image = 0;
		else image = 4;
	}
}

//X軸移動
void WalkingEnemy2::MoveX(Map *M) {
	pos.x += speed.x;
	if (speed.x > 0.) {
		if (MapHitRB(M) == 1 || MapHitRU(M) == 1) {
			if (MapKillRB(M) == 2 || MapKillRB(M) == 3 || MapKillRU(M) == 2 || MapKillRU(M) == 3) GetDamage(0);
			if (MapKillRB(M) == 5 || MapKillRB(M) == 6 || MapKillRU(M) == 5 || MapKillRU(M) == 6) Killed();
			RevisXR(M);
			dire *= -1;
		} else if(MapHitCBP1(M) == 0 && (MapHitLBP1(M) == 1 || MapHitLBP1(M) == 11 || MapHitLBP1(M) == 12) && speed.y < 1.){
			dire *= -1;
		}
	}
	if (speed.x < 0.) {
		dire = -1;
		if (MapHitLB(M) == 1 || MapHitLU(M) == 1) {
			if (MapKillLB(M) == 2 || MapKillLB(M) == 3 || MapKillLU(M) == 2 || MapKillLU(M) == 3) GetDamage(0);
			if (MapKillLB(M) == 5 || MapKillLB(M) == 6 || MapKillLU(M) == 5 || MapKillLU(M) == 6) Killed();
			RevisXL(M);
			dire *= -1;
		} else if (MapHitCBP1(M) == 0 && (MapHitRBP1(M) == 1 || MapHitRBP1(M) == 11 || MapHitRBP1(M) == 12) &&speed.y < 1.) {
			dire *= -1;
		}
	}
	if (pos.x + gap.x <= 0.) {
		pos.x = -gap.x;
		dire *= -1;
	}
	if (pos.x + OBJSIZEX - gap.x >= M->GetWorldSize().x*MAPSIZEX) {
		pos.x = M->GetWorldSize().x*MAPSIZEX - OBJSIZEX + gap.x;
		dire *= -1;
	}
}

CarrotMan::CarrotMan(int x, int y) {
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = 0.0;
	gap.x = 8.0;
	gap.y = 1.0;
	time = 0;
	image = 0;
	BaseImg = 12*2;
	HP = 1;
	dire = -1;
	alive = true;
	Press = 0;
	PDecision = 0;
	EDecision = 0;
}

void CarrotMan::animation() {
	if (Press != 0 && time >= 0) {
		if (dire == -1)image = time / 12;
		if (dire == 1)image = time / 12 + 4;
		time++;
		if (time >= 12 * 4) {
			time = 0;
			if (dire == -1)image = 0;
			else image = 4;
		}
	}else if (Press != 0 && time <0) {
		image = 10;
	}else if (Press == 0) {
		image = 8;
	}
}
void CarrotMan::update(Map *M) {
	//画面内に存在してゲーム上に存在する場合
	if (InScreen(M) && OnField) {
		if (Press == 1) {
			UpdateSpeedX();
			MoveX(M);
			UpdateSpeedY();
			MoveY(M);
			BlockKilled(M);
		}
		else if(Press == 0){
			if (abs(PlayerManager::GetInstance().GetPlayerX() - pos.x) <= 32*3) {
				time++;
				if (time > 30) {
					Press = 1;
					speed.y = -10;
					PDecision = 1;
					EDecision = 2;
					time = -1;
					SoundManager::GetInstance().PlaySE(EXPOSE);
				}
			} else {
				time = 0;
			}
		}
		animation();
	}
	else {
		//画面外にいる場合は一旦消去
		OnField = false;
	}
	//一旦消去されて、初期位置が画面外にある場合
	if (!OnField && !InitInScreen(M)) {
		ReturnInitPos();		//元の位置に戻してやる
		OnField = true;			//存在を戻してやる
		Press = 0;
		PDecision = 0;
		EDecision = 0;
		speed.x = 0;
		if (PlayerManager::GetInstance().GetPlayerX() > initpos.x)dire = 1;	//主人公が初期位置より右側にいるなら右側を向かせる
		else dire = -1;														//そうでなければ左側を向かせる
	}
}


void CarrotMan::UpdateSpeedX() {
	if(Press != 0 && time >= 0)speed.x = (float)dire*2.0;
}

void CarrotMan::Landing(Map *M) {
	RevisYB(M);
	speed.y = 0.;
	Land = true;
	if (time < 0) {
		if (PlayerManager::GetInstance().GetPlayerX() > pos.x)dire = 1;	//主人公が初期位置より右側にいるなら右側を向かせる
		else dire = -1;
		time = 0;
		Press = 1;
	}
}

//亀
BallSlime::BallSlime(int x, int y) {
	Combo = 0;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = 0.0;
	gap.x = 2.0;
	gap.y = 1.0;
	time = 0;
	image = 0;
	BaseImg = 12*3;
	HP = 1;
	dire = -1;
	alive = true;
	Mode = 0;
	Press = 1;
	PDecision = 1;
	EDecision = 2;
	DecTime = 0;
	BKTime = 0;
}

void BallSlime::HitBlock(Map *M) {
	if (Mode != 0) {
		M->Hited(LeftXMap(), UpYMap());
		M->Hited(RightXMap(), UpYMap());
	}
}
void BallSlime::HitBlockL(Map *M) {
	if (Mode != 0) {
		M->Hited(LeftXMap(), UpYMap());
		M->Hited(LeftXMap(), BottomYMap());
	}
}
void BallSlime::HitBlockR(Map *M) {
	if (Mode != 0) {
		M->Hited(RightXMap(), UpYMap());
		M->Hited(RightXMap(), BottomYMap());
	}
}

void BallSlime::BlockKilled(Map *M) {
	if (BottomLBlockKill(M) == 2 || BottomLBlockKill(M) == 3 || BottomRBlockKill(M) == 2 || BottomRBlockKill(M) == 3) {
		if (BKTime <= 0) {
			if (Mode == 0 || Mode == 2) {
				GameData::GetInstance().PlusScore(100);
				ObjectManager::GetInstance().MakeScoreEffect(0, getX(), getY() - 16);
			}
			Combo = 0;
			SoundManager::GetInstance().PlaySE(THREAD);
			speed.y = -6;
			BKTime = 1;
			Mode = 1;
		}
	}
}

void BallSlime::animation() {
	if (Mode == 0) {
		if (dire == -1)image = time / 12;
		if (dire == 1)image = time / 12 + 4;
		time++;
		if (time >= 12 * 4) {
			time = 0;
			if (dire == -1)image = 0;
			else image = 4;
		}
	}
	if (Mode == 1)image = 8;
	if (Mode == 2) {
		image = 8 + time / 6;
		time++;
		if (time >= 6 * 4) {
			time = 0;
			image = 8;
		}
	}
}

//主人国に踏まれた時
void BallSlime::Treaded() {
	if (Mode == 0 || Mode == 2) {
		Mode = 1;
		SoundManager::GetInstance().PlaySE(THREAD);
		GameData::GetInstance().PlusScore(CalcScore(PlayerManager::GetInstance().GetCombo()));
		ObjectManager::GetInstance().MakeScoreEffect(PlayerManager::GetInstance().GetCombo(), getX(), getY() - 16);
		Combo = 0;
		DecTime = 0;
	}else if (Mode == 1) {
		if (DecTime >= 60 * JumpTime) {
			DecTime = 0;
			SoundManager::GetInstance().PlaySE(THREAD);
			GameData::GetInstance().PlusScore(CalcScore(PlayerManager::GetInstance().GetCombo()));
			ObjectManager::GetInstance().MakeScoreEffect(PlayerManager::GetInstance().GetCombo(), getX(), getY() - 16);
		}
	}
}

void BallSlime::UpdateSpeedX() {
	if (Mode == 0)speed.x = (float)dire*2.0;
	if (Mode == 1)speed.x = 0.;
	if (Mode == 2)speed.x = (float)dire*8.0;
}

void BallSlime::ColliSide2Obj(float x, float y) {
	if (Mode == 0) {
		if (x > pos.x && dire == 1)dire = -1;
		if (x <= pos.x && dire == -1)dire = 1;
	}
}

void BallSlime::ColliSide2ObjP(float x, float y) {
	if (Mode == 0) {
		if (x > pos.x && dire == 1)dire = -1;
		if (x <= pos.x && dire == -1)dire = 1;
		DecTime = 0;
	}
	if (Mode == 1) {
		if (pos.x < x)dire = -1;
		else dire = 1;
		DecTime = 0;
		Mode = 2;
		SoundManager::GetInstance().PlaySE(HIT);
	}
}

void BallSlime::update(Map *M) {
	if (BKTime > 0) {
		BKTime++;
		if (BKTime >= 10)BKTime = 0;
	}
	//画面内に存在してゲーム上に存在する場合
	if (InScreen(M) && OnField) {
		if (Mode == 0) {
			PDecision = 1;
			EDecision = 2;
			Press = 1;
		}
		if (Mode == 1) {
			PDecision = 2;
			EDecision = 2;
			Press = 0;
			DecTime++;
			if (DecTime >= 60 * JumpTime) {
				if (Land) {
					speed.y = -3.;
					Land = false;
				}
				Press = 1;
			}
			if (DecTime >= 60 * FukkatsuTime) {
				Mode = 0;
				DecTime = 0;
			}
		}
		if (Mode == 2) {
			DecTime++;
			if (DecTime >= 30) {
				PDecision = 1;
				EDecision = 1;
				Press = 1;
			}
			else {
				Press = 0;
				PDecision = 2;
				EDecision = 1;
			}
		}

		UpdateSpeedX();
		MoveX(M);
		UpdateSpeedY();
		MoveY(M);
		BlockKilled(M);
		animation();
	}
	else {
		//画面外にいる場合は一旦消去
		OnField = false;
	}
	//一旦消去されて、初期位置が画面外にある場合
	if (!OnField && !InitInScreen(M)) {
		ReturnInitPos();		//元の位置に戻してやる
		OnField = true;			//存在を戻してやる
		Mode = 0;
		DecTime = 0;
		if (PlayerManager::GetInstance().GetPlayerX() > initpos.x)dire = 1;	//主人公が初期位置より右側にいるなら右側を向かせる
		else dire = -1;														//そうでなければ左側を向かせる
	}
}


BallSlime2::BallSlime2(int x, int y) {
	Combo = 0;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = 0.0;
	gap.x = 2.0;
	gap.y = 1.0;
	time = 0;
	image = 0;
	BaseImg = 12 * 4;
	HP = 1;
	dire = -1;
	alive = true;
	Mode = 0;
	Press = 1;
	PDecision = 1;
	EDecision = 2;
	DecTime = 0;
}

void BallSlime2::MoveX(Map *M) {
	pos.x += speed.x;
	if (speed.x > 0.) {
		if (MapHitRB(M) == 1 || MapHitRU(M) == 1) {
			if (MapKillRB(M) == 2 || MapKillRB(M) == 3 || MapKillRU(M) == 2 || MapKillRU(M) == 3) GetDamage(0);
			if (MapKillRB(M) == 5 || MapKillRB(M) == 6 || MapKillRU(M) == 5 || MapKillRU(M) == 6) Killed();
			RevisXR(M);
			dire *= -1;
		}
		else if (MapHitCBP1(M) == 0 && (MapHitLBP1(M) == 1 || MapHitLBP1(M) == 11 || MapHitLBP1(M) == 12) && speed.y < 1.) {
			if(Mode == 0)dire *= -1;
		}
	}
	if (speed.x < 0.) {
		dire = -1;
		if (MapHitLB(M) == 1 || MapHitLU(M) == 1) {
			if (MapKillLB(M) == 2 || MapKillLB(M) == 3 || MapKillLU(M) == 2 || MapKillLU(M) == 3) GetDamage(0);
			if (MapKillLB(M) == 5 || MapKillLB(M) == 6 || MapKillLU(M) == 5 || MapKillLU(M) == 6) Killed();
			RevisXL(M);
			dire *= -1;
		}
		else if (MapHitCBP1(M) == 0 && (MapHitRBP1(M) == 1 || MapHitRBP1(M) == 11 || MapHitRBP1(M) == 12) && speed.y < 1.) {
			if (Mode == 0)dire *= -1;
		}
	}
	if (pos.x + gap.x <= 0.) {
		pos.x = -gap.x;
		dire *= -1;
	}
	if (pos.x + OBJSIZEX - gap.x >= M->GetWorldSize().x*MAPSIZEX) {
		pos.x = M->GetWorldSize().x*MAPSIZEX - OBJSIZEX + gap.x;
		dire *= -1;
	}
}