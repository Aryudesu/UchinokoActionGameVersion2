#pragma once
#include "Conf.h"
#include "DxLib.h"
#include "InputKey.h"
#include "Variable.h"
#include "ImageManager.h"
#include "SoundManager.h"
#include "function.h"
#include "Object.h"
#include "Map.h"
#include "Player.h"
#include "PlayerManager.h"
#include "ObjectManager.h"

Player::Player() {
	Combo = 0;
	initpos.x = pos.x = 0.;	//initposは初期位置　posは現在位置
	initpos.y = pos.y = 0.;	//initposは初期位置　posは現在位置
	gap.x = 8.0;					//オブジェクトサイズはOBJSIZE分あるがx軸方向に対する見た目と実際の判定の大きさ（両側から同じ分引く計算）
	gap.y = 0.0;					//オブジェクトサイズはOBJSIZE分あるがy軸方向に対する見た目と実際の判定の大きさ（上からだけ引く計算）
	Vmax.x = 4.0;					//x軸方向に対する最大速度
	Vmax.y = 12.0;					//y軸方向に対する最大速度
	speed.x = 0.;					//初期設定速度
	speed.y = 0.;					//初期設定速度
	dire = 1;						//初期に向いている方向
	HP = 1;							//初期HP
	time = 0;						//アニメとかのカウント用
	image = 0;						//グラフィック用
	ImgCount = 0;
	JumpCount = 0;					//ジャンプ回数カウント用（２段ジャンプとかの多段ジャンプを考えるためのもの）
	JumpFlag = false;				//ジャンプボタン押した判定用
	Damage = false;					//ダメージ受けた時
	Ladd = false;					//ハシゴ乗ってるかどうか
	Tobiori = false;
	Dead = false;
	Moving = 0;
	MovingTime = 0;
	Beat = false;
	GravDire = 1;
}

void Player::Init(float x,float y) {
	pos.x = (float)x;	//initposは初期位置　posは現在位置
	pos.y = (float)y;	//initposは初期位置　posは現在位置
	Moving = 0;
	MovingTime = 0;
}

void Player::SetInitPos(float x,float y) {
	Init(x, y);
	initpos.x = (float)x;	//initposは初期位置
	initpos.y = (float)y;	//initposは初期位置
}

//X軸速度更新
void Player::UpdateSpeedX() {
	//左が押された時
	if (ReturnKey(KEY_INPUT_LEFT) != 0) {
		if (ReturnKey(KEY_INPUT_X) == 0) {
			if (speed.x <= -Vmax.x) {
				if(!Water)speed.x += ACCE * D2WA;
				else speed.x += ACCE * D2WA * 0.3;
			}
		}
		else {
			if (speed.x <= -Vmax.x * 1.5) {
				if (!Water)speed.x = -Vmax.x * DASH;
				else speed.x = -Vmax.x * DASH * 0.1;
			}
		}
		if (speed.x > 0)speed.x *= 0.95;
		if (!Water)speed.x -= ACCE;
		else speed.x -= ACCE * 0.3;
	}
	//右が押された時
	if (ReturnKey(KEY_INPUT_RIGHT) != 0) {
		if (ReturnKey(KEY_INPUT_X) == 0) {
			if (speed.x >= Vmax.x) {
				if (!Water)speed.x -= ACCE * D2WA;
				else speed.x -= ACCE * D2WA * 0.3;
			}
		}else {
			if (speed.x >= Vmax.x * 1.5) {
				if (!Water)speed.x = Vmax.x * DASH;
				else speed.x = Vmax.x * DASH * 0.1;
			}
		}
		if (speed.x < 0)speed.x *= 0.95;
		if (!Water)speed.x += ACCE;
		else speed.x += ACCE * 0.3;
	}
	//何も押さない時か両方押した時
	if ((!ReturnKey(KEY_INPUT_LEFT) && !ReturnKey(KEY_INPUT_RIGHT)) || (ReturnKey(KEY_INPUT_LEFT) && ReturnKey(KEY_INPUT_RIGHT))) {
		speed.x *= 0.93;
		if (AbsF(speed.x) <= ACCE * 2.)speed.x = 0;
	}
}

//ジャンプ関連アレコレ
void Player::Jump() {
	//着地したりハシゴに捕まっていたらジャンプ回数0
	if (Land || Ladd || Water)JumpCount = 0;
	//崖から降りたりしたらジャンプ回数1（ジャンプ回数0は地面からの直接ジャンプ扱い）
	else if (JumpCount == 0)JumpCount = 1;
	//ジャンプ
	//ジャンプボタン押してない && Zキー押した && ジャンプ回数0以上指定回数未満
	if (!JumpFlag && ReturnKey(KEY_INPUT_Z) > 0 && JumpCount >= 0 && JumpCount < MSJUMP) {
		//横方向に速さが乗っていない場合
		if (AbsF(speed.x) < Vmax.x * 2. / 3.) {
			if (JumpCount == 0)speed.y = -11 * GravDire;	//通常ジャンプ
			if (JumpCount >  0)speed.y = -9 * GravDire;	//空中ジャンプの時はちょっと初速度少なめ．
												//if(JumpCount == 2)speed.y = -8;	//仮に３段目以降も速度を変えたい場合はこのように記述する
		}
		//横方向の勢いがある場合
		else {
			if (JumpCount == 0)speed.y = -12 * GravDire;	//通常ジャンプ
			if (JumpCount >  0)speed.y = -10 * GravDire;	//空中ジャンプの時はちょっと初速度少なめ．
												//if(JumpCount == 2)speed.y = -9;	//仮に３段目以降も速度を変えたい場合はこのように記述する
		}
		if (Water) {
			if (!ReturnKey(KEY_INPUT_DOWN) && !ReturnKey(KEY_INPUT_UP))speed.y = -10 * .4;
			if (!ReturnKey(KEY_INPUT_DOWN) && ReturnKey(KEY_INPUT_UP))speed.y  = -10 * .6;
			if (ReturnKey(KEY_INPUT_DOWN) && !ReturnKey(KEY_INPUT_UP))speed.y  = -10 * .2;
			time = 1;
		}
		if(JumpCount == 0)SoundManager::GetInstance().PlaySE(JUMP1);
		if(JumpCount >  0)SoundManager::GetInstance().PlaySE(JUMP2);
		if (!JumpFlag)JumpCount++;			//ジャンプ回数をカウント
		Land = false;						//ジャンプしたので地面に立ってない
		JumpFlag = true;					//ジャンプボタンは押した
		Ladd = false;				//ハシゴから降りる
	}
	//先行入力も考慮して0Fではなく5F超過押してたらジャンプボタン押した判定にする
	//ジャンプする時に「ジャンプボタンを押してない場合」にジャンプするという設定だが
	//『ボタンを押す　→　（5F以内に）着地　→　即ジャンプ』
	//というスムーズな流れができるようになる
	if (ReturnKey(KEY_INPUT_Z) >= 5) {
		JumpFlag = true;
	}
	//ジャンプボタン外した時
	if (ReturnKey(KEY_INPUT_Z) == 0) {
		if (speed.y < 0 && GravDire ==  1)speed.y *= 0.75;	//上方向に速度があれば減速
		if (speed.y > 0 && GravDire == -1)speed.y *= 0.75;	//上方向に速度があれば減速
		JumpFlag = false;					//ジャンプボタン外した判定を外す
	}
}

//Y軸方向の速さ更新
void Player::UpdateSpeedY() {
	//ハシゴアクションしてない時
	//重力
	if (!Water) {
		if (GravDire == 1) {
			speed.y += GRAVITY;
			if (speed.y >= Vmax.y)speed.y = Vmax.y;
		} else {
			speed.y -= GRAVITY;
			if (speed.y <= -Vmax.y)speed.y = -Vmax.y;
		}
	} else {
		if (GravDire == 1) {
			speed.y += GRAVITY / 3.;
			if (speed.y >= Vmax.y / 2.)speed.y = Vmax.y / 2.;
		} else {
			speed.y -= GRAVITY / 3.;
			if (speed.y <= -Vmax.y / 2.)speed.y = -Vmax.y / 2.;
		}
	}
	//終端速度
	if(!Damage)Jump();//ジャンプ

	//崖から落ちた時（速度１以上）は着地してない判定にする
	if (AbsF(speed.y) >= 1. + GRAVITY)Land = false;
}

//X軸移動
void Player::MoveX(Map *M) {
	VECTOR tmp = pos,tmp2;
	pos.x += speed.x;
	tmp2 = pos;

	if (ReturnKey(KEY_INPUT_RIGHT) && Land) {
		if (MapHitRU(M) == 18 || MapHitRB(M) == 19) {
			Moving = 3;
			MovingTime = 0;
		}
	}
	if (ReturnKey(KEY_INPUT_LEFT) && Land) {
		if (MapHitLU(M) == 20 || MapHitLB(M) == 21) {
			Moving = 4;
			MovingTime = 0;
		}
	}
	if (speed.x > 0.) {
		dire = 1;
		if (MapHitRB(M) == 1 || MapHitRU(M) == 1 || (MapHitRB(M) >= 14 && MapHitRB(M) <= 21) || (MapHitRU(M) >= 14 && MapHitRU(M) <= 21)) {
			if (MapKillRB(M) == 1 || MapKillRB(M) == 3 || MapKillRU(M) == 1 || MapKillRU(M) == 3) Damaged(-dire, 0);
			if (MapKillRB(M) == 4 || MapKillRB(M) == 6 || MapKillRU(M) == 4 || MapKillRU(M) == 6) Death();
			RevisXR(M);
			speed.x = 0;
		}
	}
	else if (speed.x < 0.) {
		dire = -1;
		if (MapHitLB(M) == 1 || MapHitLU(M) == 1 || (MapHitLB(M) >= 14 && MapHitLB(M) <= 21) || (MapHitLU(M) >= 14 && MapHitLU(M) <= 21)) {
			if (MapKillLB(M) == 1 || MapKillLB(M) == 3 || MapKillLU(M) == 1 || MapKillLU(M) == 3) Damaged(-dire, 0);
			if (MapKillLB(M) == 4 || MapKillLB(M) == 6 || MapKillLU(M) == 4 || MapKillLU(M) == 6) Death();
			RevisXL(M);
			speed.x = 0;
		}
	}
	if (M->GetSLU().x > pos.x + gap.x) {
		pos.x = M->GetSLU().x - gap.x;
		if (MapHitRB(M) == 1 || MapHitRU(M) == 1 || (MapHitRB(M) >= 14 && MapHitRB(M) <= 21) || (MapHitRU(M) >= 14 && MapHitRU(M) <= 21))Death(1);
	}
	if (pos.x + OBJSIZEX - gap.x >= M->GetSLU().x + WINDOWX) {
		pos.x = M->GetSLU().x + WINDOWX - OBJSIZEX + gap.x;
		if (MapHitLB(M) == 1 || MapHitLU(M) == 1 || (MapHitLB(M) >= 14 && MapHitLB(M) <= 21) || (MapHitLU(M) >= 14 && MapHitLU(M) <= 21))Death(1);
	} 
}

//Y軸移動
void Player::MoveY(Map *M) {
	bool BWater = Water;
	pos.y += speed.y;
	Water = (MapHitCC(M) == 5);
	if (BWater != Water) {
		for (int i = 0; i < 100; i++) {
			int tmpy = 0;
			if (speed.y < 0)tmpy = (int)((pos.y + OBJSIZEY / 2) / MAPSIZEY + 1) * 32 - 1;
			if (speed.y > 0)tmpy = (int)((pos.y + OBJSIZEY / 2) / MAPSIZEY) * 32 - 1;
			ObjectManager::GetInstance().MakeEffectObj(0, pos.x + OBJSIZEX / 2, tmpy);
		}
		SoundManager::GetInstance().PlaySE(WATER);
		if ((speed.y < 0 && GravDire == 1) || (speed.y > 0 && GravDire == -1))speed.y = speed.y * 2.5;
	}
	if (MapHitCC(M) == 7)GravDire = -1;
	if (MapHitCC(M) == 8)GravDire =  1;
	if (!Dead) {
		if (ReturnKey(KEY_INPUT_DOWN)) {
			if (MapHitLB(M) == 14 && MapHitRB(M) == 15) {
				Moving = 1;
				MovingTime = 0;
			}
		}
		if (ReturnKey(KEY_INPUT_UP)) {
			if (MapHitLU(M) == 16 && MapHitRU(M) == 17) {
				Moving = 2;
				MovingTime = 0;
			}
		}
	}
	//着地
	if (GravDire == 1) {
		if ((MapHitLB(M) == 1 || MapHitRB(M) == 1) || (MapHitLB(M) >= 14 && MapHitLB(M) <= 21) || (MapHitRB(M) >= 14 && MapHitRB(M) <= 21)) {
			if (MapKillLB(M) == 1 || MapKillRB(M) == 3) Damaged(-dire, 0);
			if (MapKillLB(M) == 4 || MapKillRB(M) == 6) Death();
			if (!Dead)RevisYB(M);
			if (!Dead)speed.y = 0.;
			Land = true;
		}
	} else {
		if ((MapHitLU(M) == 1 || MapHitRU(M) == 1) || (MapHitLU(M) >= 14 && MapHitLU(M) <= 21) || (MapHitRU(M) >= 14 && MapHitRU(M) <= 21)) {
			if (MapKillLU(M) == 1 || MapKillRU(M) == 3) Damaged(-dire, 0);
			if (MapKillLU(M) == 4 || MapKillRU(M) == 6) Death();
			if (!Dead)RevisYU(M);
			if (!Dead)speed.y = 0.;
			Land = true;
		}
	}
	//下から登れる
	if (GravDire == 1) {
		if (MapHitLB(M) == 11 || MapHitRB(M) == 11) {
			if ((int)(pos.y + (float)OBJSIZEY - 1.) % MAPSIZEY <= (int)abs(speed.y) && speed.y > 0.) {
				if (!Dead)RevisYB(M);
				if (!Dead)speed.y = 0.;
				Land = true;
			}
		}
	} else {
		if (MapHitLU(M) == 11 || MapHitRU(M) == 11) {
			if ((int)pos.y % MAPSIZEY >= (MAPSIZEY - (int)abs(speed.y)) && speed.y > 0.) {
				if (!Dead)RevisYU(M);
				if (!Dead)speed.y = 0.;
				Land = true;
			}
		}
	}
	//下を押すと降りれる床から飛び降りアクション
	if (GravDire == 1) {
		if (MapHitLB(M) == 12 || MapHitRB(M) == 12) {
			if (ReturnKey(KEY_INPUT_DOWN))Tobiori = true;
			if ((int)(pos.y + (float)OBJSIZEY - 1.) % MAPSIZEY <= (int)abs(speed.y) && speed.y > 0. && !Tobiori) {
				if (!Dead)RevisYB(M);
				if (!Dead)speed.y = 0.;
				Land = true;
			}
		}
		if ((int)(pos.y + (float)OBJSIZEY - 1.) % MAPSIZEY > (int)abs(speed.y) && Tobiori == true)Tobiori = false;
	} else {
		if (MapHitLU(M) == 12 || MapHitRU(M) == 12) {
			if (ReturnKey(KEY_INPUT_UP))Tobiori = true;
			if ((int)pos.y % MAPSIZEY >= (MAPSIZEY - (int)abs(speed.y)) && speed.y > 0. && !Tobiori) {
				if (!Dead)RevisYU(M);
				if (!Dead)speed.y = 0.;
				Land = true;
			}
		}
		if ((int)pos.y % MAPSIZEY < (MAPSIZEY - (int)abs(speed.y)) && Tobiori == true)Tobiori = false;
	}
	//頭ごっつんダメージ
	if (GravDire == 1) {
		if (MapHitLU(M) == 1 || MapHitRU(M) == 1 || (MapHitLU(M) >= 14 && MapHitLU(M) <= 21) || (MapHitRU(M) >= 14 && MapHitRU(M) <= 21)) {
			if (MapKillLU(M) == 1 || MapKillRU(M) == 3) Damaged(-dire, 0);
			if (MapKillLU(M) == 4 || MapKillRU(M) == 6) Death();
			if (!Dead)RevisYU(M);
			if (!Dead)speed.y = 0.5;
		}
	} else {
		if (MapHitLB(M) == 1 || MapHitRB(M) == 1 || (MapHitLB(M) >= 14 && MapHitLB(M) <= 21) || (MapHitRB(M) >= 14 && MapHitRB(M) <= 21)) {
			if (MapKillLB(M) == 1 || MapKillRB(M) == 3) Damaged(-dire, 0);
			if (MapKillLB(M) == 4 || MapKillRB(M) == 6) Death();
			if (!Dead)RevisYB(M);
			if (!Dead)speed.y = 0.5;
		}
	}
	//頭ごっつん
	if (GravDire == 1) {
		if (MapHitLU(M) == 13 || MapHitRU(M) == 13) {
			if ((int)pos.y % MAPSIZEY >= (int)abs(32. + speed.y) && speed.y < 0) {
				if (!Dead)RevisYU(M);
				if (!Dead)speed.y = 0.5;
			}
		}
	} else {
		if (MapHitLB(M) == 13 || MapHitRB(M) == 13) {
			if ((int)(pos.y + (float)OBJSIZEY - 1.) % MAPSIZEY <= (int)abs(speed.y) && speed.y < 0) {
				if (!Dead)RevisYB(M);
				if (!Dead)speed.y = 0.5;
			}
		}
	}
	if (Land && Combo > 0)Combo = 0;
	//上下繋がってる
	if (M->GetScrollMode() == 5) {
		if (pos.y > M->GetSLU().y + WINDOWY)pos.y = M->GetSLU().y - OBJSIZEY;
		if (pos.y + OBJSIZEY < M->GetSLU().y)pos.y = M->GetSLU().y + WINDOWY;
	}else{
		//上下繋がってなければ落ちたら死ぬ
		if (GravDire == 1) {
			if (pos.y > M->GetSLU().y + WINDOWY + OBJSIZEY * 3)Death(2);
		}
		else {
			if (pos.y + OBJSIZEY < M->GetSLU().y - OBJSIZEY * 3)Death(2);
		}
	}
}

//アニメーション
void Player::animation() {
	//やられた時
	if (Dead) {
		time++;
		if (time >= 5) {
			ImgCount++;
			if (ImgCount >= 4)ImgCount = 0;
			time = 0;
		}
		image = 20*4 + ImgCount;
		return;
	}
	//土管とか使ってる時
	if (Moving != 0) {
		if (Moving % 10 == 1 || Moving % 10 == 2) {
			if (dire > 0)image = 0;
			else image = 4;
		} 
		if (Moving % 10 == 3 || Moving % 10 == 4){
			if (Moving % 10 == 3)image = 3 * 4 + ((MovingTime / 5) % 4);
			if (Moving % 10 == 3)image = 2 * 4 + ((MovingTime / 5) % 4);
		}
		return;
	}
	//水中
	if (Water && !Land) {
		if (time > 0)time++;
		if (time >= 6) {
			ImgCount++;
			time = 1;
			if (ImgCount >= 4) {
				ImgCount = 0;
				time = 0;
			}
		}
		if (dire == 1)image = 9 * 4 + ImgCount;
		if (dire == -1)image = 10 * 4 + ImgCount;
		return;
	}
	//通常モーションの時
	if (!Damage && !Ladd) {
		//着地時
		if (Land) {
			//右方向
			if (speed.x > 1.) {
				//歩いてる時
				if (AbsF(speed.x) <= Vmax.x) { image = 2 * 4 + ImgCount; }
				else { image = 4 * 4 + ImgCount; }
			}
			//左方向
			if (speed.x < -1.) {
				//歩いてる時
				if (AbsF(speed.x) <= Vmax.x) { image = 3 * 4 + ImgCount; }
				else { image = 5 * 4 + ImgCount; }
			}
			//止まった時
			if (speed.x <= 1. && speed.x >= -1.) {
				if (dire ==  1)image = 0 * 4 + ImgCount;
				if (dire == -1)image = 1 * 4 + ImgCount;
			}
		}
		//ジャンプしたりした時
		else {
			//上昇中
			if (speed.y < 0.) {
				if (dire == 1)image = 6 * 4 + ImgCount % 2;
				if (dire == -1)image = 7 * 4 + ImgCount % 2;
			}
			//下降中
			if (speed.y > 0.) {
				if (dire == 1)image = 6 * 4 + ImgCount % 2 + 2;
				if (dire == -1)image = 7 * 4 + ImgCount % 2 + 2;
			}
		}
		time++;
		if (time >= 5) {
			ImgCount++;
			if (ImgCount >= 4)ImgCount = 0;
			time = 0;
		}
		return;
	}
	if(Damage){
		//被ダメモーションのとき
		if (dire ==  1)image = 15 * 4 + DamageTime / 4;
		if (dire == -1)image = 14 * 4 + DamageTime / 4;
		return;
	}
	if (Ladd) {
		//ハシゴモーションのとき
		if (speed.y != 0) {
			image = 8 * 4 + ImgCount;
		}
		else {
			image = 8 * 4;
		}
		time++;
		if (time >= 5) {
			ImgCount++;
			if (ImgCount >= 4)ImgCount = 0;
			time = 0;
		}
		return;
	}
}

//描画
void Player::draw(Map *M) {
	int TurnY = (GravDire == 1)?FALSE:TRUE;
	if (Moving == 0 || Moving == -2)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), MAIN_CHARA, image, TRUE,TurnY);
	if (Moving != 0) {
		if (Moving ==  1)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY() + MovingTime, MAIN_CHARA, image, TRUE, TurnY);
		if (Moving ==  2)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY() - MovingTime, MAIN_CHARA, image, TRUE, TurnY);
		if (Moving ==  3)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX() + MovingTime, pos.y - M->GetScreenLUY(), MAIN_CHARA, image, TRUE, TurnY);
		if (Moving ==  4)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX() - MovingTime, pos.y - M->GetScreenLUY(), MAIN_CHARA, image, TRUE, TurnY);
		if (Moving == 11)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY() - MovingTime + OBJSIZEY, MAIN_CHARA, image, TRUE, TurnY);
		if (Moving == 12)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY() + MovingTime - OBJSIZEY, MAIN_CHARA, image, TRUE, TurnY);
		if (Moving == 13)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX() - MovingTime + OBJSIZEX, pos.y - M->GetScreenLUY(), MAIN_CHARA, image, TRUE, TurnY);
		if (Moving == 14)ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX() + MovingTime - OBJSIZEX, pos.y - M->GetScreenLUY(), MAIN_CHARA, image, TRUE, TurnY);
	}
}

void Player::DeadMotion(Map *M) {
	if (Moving == 0) {
		UpdateSpeedY();
		MoveY(M);
	}
	animation();
}

void Player::DamageMotion(Map *M) {
	DamageUpdate();
	if (Moving == 0) {
		MoveX(M);
		UpdateSpeedY();
		MoveY(M);
		TouchedBlock(M);
	}
	animation();
}

void Player::LadderMotion(Map *M) {
	Move(M);
	if (Moving == 0) {
		MoveX(M);
		UpdateSpeedY();
		LadderAct(M);
		MoveY(M);
		TouchedBlock(M);
	}
	animation();
}

void Player::NormalMotion(Map *M) {
	UpdateSpeedX();
	Move(M);
	if (Moving == 0) {
			MoveX(M);
			UpdateSpeedY();
			LadderAct(M);
			MoveY(M);
			TouchedBlock(M);
	}
	animation();
}

//更新
void Player::update(Map *M) {
	if (Dead) {
		DeadMotion(M);
		return;
	}
	if (Damage) {
		DamageMotion(M);
		return;
	}
	if (Ladd) {
		LadderMotion(M);
		return;
	}
	NormalMotion(M);
}

int Player::GetHP() { return PlayerManager::GetInstance().GetHP(); }

void Player::HitBlock(Map *M) {
	if (GravDire == 1) {
		M->Hited(LeftXMap(), UpYMap());
		M->Hited(RightXMap(), UpYMap());
	}
}

void Player::HitBlockB(Map* M) {
	if (GravDire == -1) {
		M->Hited(LeftXMap(), UpYMap());
		M->Hited(RightXMap(), UpYMap());
	}
}

void Player::TouchedBlock(Map *M) {
	M->Touched(LeftXMap(),UpYMap());
	M->Touched(LeftXMap(), BottomYMap());
	M->Touched(RightXMap(), UpYMap());
	M->Touched(RightXMap(), BottomYMap());
}

//ハシゴ
bool Player::Lad(Map *M) {
	return (MapHitRB(M) == 10 && MapHitLB(M) == 10 && MapHitRU(M) == 10 && MapHitLU(M) == 10);
}

void Player::LadderAct(Map *M) {
	if (Lad(M) && ReturnKey(KEY_INPUT_UP))Ladd = true;
	if (Lad(M) && !Land && ReturnKey(KEY_INPUT_DOWN))Ladd = true;
	if (!Lad(M) || (Land && ReturnKey(KEY_INPUT_DOWN)))Ladd = false;
	if (Ladd) {
		if (ReturnKey(KEY_INPUT_RIGHT) && !ReturnKey(KEY_INPUT_LEFT))speed.x =  2;
		if (!ReturnKey(KEY_INPUT_RIGHT) && ReturnKey(KEY_INPUT_LEFT))speed.x = -2;
		if (!ReturnKey(KEY_INPUT_RIGHT) && !ReturnKey(KEY_INPUT_LEFT))speed.x = 0;
		if (ReturnKey(KEY_INPUT_UP) && !ReturnKey(KEY_INPUT_DOWN))speed.y = -3;
		if (!ReturnKey(KEY_INPUT_UP) && ReturnKey(KEY_INPUT_DOWN))speed.y =  3;
		if (!ReturnKey(KEY_INPUT_UP) && !ReturnKey(KEY_INPUT_DOWN))speed.y = 0;
	}
}

//HPいじる用
void Player::PlusHP(int n) {
	PlayerManager::GetInstance().PlusHP(n);
}

void Player::SetHP(int n) {
	PlayerManager::GetInstance().SetHP(n);
}

//初期位置
void Player::ReturnInitPos() {
	pos.x = initpos.x;
	pos.y = initpos.y;
}

//踏んだ時
void Player::Tread(int n) {
	speed.y = -12;
}

//ダメージ受ける時
void Player::Damaged(int direction,int n) {
	if (!Damage) {
		speed.y = 0;
		dire = direction;
		Damage = true;
		DamageTime = 0;
		PlusHP(-1);
		DamageNum = 1;
		SoundManager::GetInstance().PlaySE(DAMAG);
	}
}

//ダメージ受けた時の関数
void Player::DamageUpdate() {
	DamageTime++;
	if (DamageTime < 16) {
		speed.x = 3.*(float)dire;
	}
	if (DamageTime >= 16) {
		speed.x = 0;
		Damage = false;
		dire = -dire;
	}
}

void Player::Death(int Num) {
	if (!Dead) {
		SoundManager::GetInstance().PlaySE(DEAD);
		SoundManager::GetInstance().StopBGM(BGM1);
		if(Num!=2)speed.y = -12;
	}
	DeadNum = Num;
	Damage = true;
	Dead = true;
}

void Player::Move(Map *M) {
	if (Moving != 0){
		if (Moving == -2) {
			Moving = 0;
			return;
		}
		if (MovingTime == 0) {
			SoundManager::GetInstance().PlaySE(PIPE);
		}
		MovingTime++;
		if (MovingTime >= 32) {
			if (Moving < 10)Moving = -1;
			else Moving = -2;
			MovingTime = 0;
		}
	}
}

int Player::GetMove() { return Moving; }
int Player::GetMoveTime() { return MovingTime; }

void Player::Emerge(int Num) {
	if (MovingTime == 0)SoundManager::GetInstance().PlaySE(PIPE);
	MovingTime++;
	Moving = Num;
}

bool Player::GetDead() { return Dead; }
int Player::GetDeadNum() { return DeadNum; }

void Player::SetBeat(bool Param) { Beat = Param; }
bool Player::GetBeat() { return Beat; }
int Player::GetGravDire() { return GravDire; }