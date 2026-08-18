#include "Block.h"
#include "ImageManager.h"
#include "PlayerManager.h"
#include "ObjectManager.h"
#include "SoundManager.h"
#include "Variable.h"
#include "Conf.h"
#include "GameData.h"
#include "function.h"


//num =  0 : 空白
//num =  1 : ブロック
//num =  5 : 水中
//num =  7 : 上方向重力ブロック
//num =  8 : 下方向重力ブロック
//num = 10 : ハシゴ
//num = 11 : 下から登れる
//num = 12 : 下から登れて降りれる
//num = 13 : 透明ブロック
//num = 14~21: 土管


Block::Block() {};

void Block::SetImg(int n) { img = n; }

int Block::GetImg() { return img; }

//描画関数
void Block::draw(float x, float y, float sx, float sy) {
	BackDraw(x, y, sx, sy);
	BlDraw(x, y, sx, sy);
}
//背景描画
void Block::BackDraw(float x, float y, float sx, float sy) {
	ImageManager::GetInstance().DrawImg(x * (float)MAPSIZEX - sx, y * (float)MAPSIZEY - sy, MAP1, img, TRUE);	//描画
}
//本体描画
void Block::BlDraw(float x, float y, float sx, float sy) {
	if(alive)ImageManager::GetInstance().DrawImg(x * (float)MAPSIZEX - sx, y * (float)MAPSIZEY - sy, MAPOBJ, BlImg, TRUE);	//描画
}

void Block::SetKill(int Num) {
	Kill = Num;
}

void Block::Touched() {}
void Block::Hited() {}			//下から叩かれた時
void Block::On() {}			//上に乗られた時
void Block::PushL() {}			//左から押された時
void Block::PushR() {}			//右から押された時

void Block::SetPos(float x, float y) {
	pos.x = x;
	pos.y = y;
}

int Block::GetNum() {
	if (alive)return num;
	else return 0;
}	//判定番号取得（存在しない場合は0を返す

int Block::GetKill() {
	if (alive)return Kill;
	else return 0;
}	//攻撃判定番号取得（存在しない場合は0を返す

////////////////
//空白ブロック//
////////////////
Empty::Empty() {
	Kill = 0;
	num = 0;
	img = 0;
	alive = false;
}
void Empty::update() {}											//何もしない
void Empty::BlDraw(float x, float y, float sx, float sy) {}		//ブロック本体は描画しないので何もしない

////////////////
//透明ブロック//
////////////////
Steals::Steals() {
	Kill = 0;
	num = 1;
	img = 0;
	alive = true;
}
void Steals::update() {}										//何もしない
void Steals::BlDraw(float x, float y, float sx, float sy) {}	//ブロック本体は描画しないので何もしない

//////////////////
//レンガブロック//
//////////////////
Bricks::Bricks() {
	Kill = 0;
	num = 1;
	img = 0;
	BlImg = 60;
	time = 0;
	alive = true;
}
void Bricks::Hited() {
	Kill = 2;
	//主人公のHP確認．HPが指定されたHP以上なら壊す
	if (PlayerManager::GetInstance().GetHP() >= BRICKHP)time = -1;
	else time = 1;
}
void Bricks::update() {
	//Hited()が動作した場合
	if (time > 0) {
		BlImg = 60 + (time-1)/2 + 5;
		time++;
		if (time >= 10) {
			time = 0;
			BlImg = 60;
			Kill = 0;
		}
	} else if (time < 0) {
		BlImg = 60 - (1 + time)/2 + 1;
		time--;
		if (time <= -8) {
			time = 0;
			BlImg = 60;
			if (alive) {
				SoundManager::GetInstance().PlaySE(BROKEN);
				GameData::GetInstance().PlusScore(10);
				for(int i=0;i<5;i++)ObjectManager::GetInstance().MakeBlockFragment(0, pos.x, pos.y);
			}
			alive = false;
			Kill = 0;
		}
	}
}


//////////
//コイン//
//////////
Coin::Coin() {
	Kill = 0;
	num = 0;
	img = 0;
	time = 0;
	BlImg = 51;
	alive = true;
}
void Coin::update() {
	BlImg = 51 + time / 8;
	time++;
	if (time >= 32) {
		BlImg = 51;
		time = 0;
	}
}
void Coin::Touched() {
	if (alive) {
		PlayerManager::GetInstance().PlusCoin(1);
		SoundManager::GetInstance().PlaySE(COIN);
		GameData::GetInstance().PlusScore(100);
		alive = false;
	}
}
//////////////
//回復コイン//
//////////////
HealingCoin::HealingCoin() {
	Kill = 0;
	num = 0;
	img = 0;
	time = 0;
	BlImg = 100;
	alive = true;
}
void HealingCoin::update() {
	BlImg = 100 + time / 8;
	time++;
	if (time >= 32) {
		BlImg = 100;
		time = 0;
	}
}
void HealingCoin::Touched() {
	if (alive) {
		PlayerManager::GetInstance().PlusHP(1);
		GameData::GetInstance().PlusScore(1000);
		ObjectManager::GetInstance().MakeScoreEffect(4, pos.x, pos.y);
		alive = false;
	}
}
/////////////
//1UPコイン//
/////////////
OneUPCoin::OneUPCoin() {
	Kill = 0;
	num = 0;
	img = 0;
	time = 0;
	BlImg = 20;
	alive = true;
}
void OneUPCoin::update() {
	BlImg = 20 + time / 8;
	time++;
	if (time >= 32) {
		BlImg = 20;
		time = 0;
	}
}
void OneUPCoin::Touched() {
	if (alive) {
		GameData::GetInstance().PlusZanki(1);
		ObjectManager::GetInstance().MakeScoreEffect(9, pos.x, pos.y);
		alive = false;
	}
}

////////////////////
//アイテムブロック//
////////////////////
ItemBlock::ItemBlock() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	BlImg = 40;
	Brown = false;
	alive = true;
}
void ItemBlock::Hited() {
	if (time >= 0 && !Brown) {
		time = -1; Kill = 2;
	}
}
void ItemBlock::update() {
	if (!Brown) {
		if (time >= 0) {
			BlImg = 40 + time / 3;
			time++;
			if (time >= 30) {
				time = 0;
			}
		} else {
			BlImg = 55 - (1+time) / 2;
			time--;
			if (time <= -10) {
				time = 0;
				BlImg = 50;
				MakeItem();
				Brown = true;
				Kill = 0;
			}
		}
	} else {
		BlImg = 50;
	}
}

//////////////////
//コインブロック//
//////////////////
CoinBlock::CoinBlock() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	BlImg = 40;
	alive = true;
}

void CoinBlock::MakeItem() {
	ObjectManager::GetInstance().MakeObject(1, pos.x, pos.y-32);
	SoundManager::GetInstance().PlaySE(COIN);
}


////////////////////
//10コインブロック//
////////////////////
Coin10Block::Coin10Block() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	count = 0;
	BlImg = 40;
	alive = true;
}
void Coin10Block::Hited() { time = -1; Kill = 2; }
void Coin10Block::update() {
	if (!Brown) {
		if (time >= 0) {
			BlImg = 40 + time / 3;
			time++;
			if (time >= 30) {
				time = 0;
			}
		}
		else {
			BlImg = 55 - (1 + time) / 2;
			time--;
			if (time <= -10) {
				time = 0;
				Kill = 0;
				BlImg = 40;
				MakeItem();
				count++;
				if (count >= 10)Brown = true;
			}
		}
	}
	else {
		BlImg = 50;
	}
}
void Coin10Block::MakeItem() {
	ObjectManager::GetInstance().MakeObject(1, pos.x, pos.y - 32);
	SoundManager::GetInstance().PlaySE(COIN);
}


////////////////////////
//回復アイテムブロック//
////////////////////////
HealingBlock::HealingBlock() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	BlImg = 40;
	alive = true;
}

void HealingBlock::MakeItem() {
	SoundManager::GetInstance().PlaySE(COIN);
	ObjectManager::GetInstance().MakeObject(2, pos.x, pos.y - 32);
}

////////////////
//茶色ブロック//
////////////////
BrownBlock::BrownBlock() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	BlImg = 50;
	alive = true;
}
void BrownBlock::update() {}

///////////////////////
//1UPアイテムブロック//
///////////////////////
OneUpBlock::OneUpBlock() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	BlImg = 40;
	alive = true;
}

void OneUpBlock::MakeItem() {
	SoundManager::GetInstance().PlaySE(EXTEND);
	ObjectManager::GetInstance().MakeObject(3, pos.x, pos.y - 32);
}


//////////////////
//ハシゴブロック//
//////////////////
LadderMakerBlock::LadderMakerBlock() {
	Kill = 0;
	num = 1;
	img = 0;
	time = 0;
	BlImg = 40;
	alive = true;
}

void LadderMakerBlock::MakeItem() {
	SoundManager::GetInstance().PlaySE(LADDER);
	ObjectManager::GetInstance().MakeObject(4, pos.x, pos.y - 32);
}

////////////////////////
//透明アイテムブロック//
////////////////////////
StealItemBlock::StealItemBlock() {
	Kill = 0;
	num = 13;
	img = 0;
	time = 0;
	BlImg = 0;
	Brown = false;
	alive = true;
}
void StealItemBlock::Hited() {
	time = -1; Kill = 2;
}
void StealItemBlock::update() {
	if (!Brown) {
		if (time >= 0) {
			BlImg = 80 + time / 3;
			time++;
			if (time >= 30) {
				time = 0;
			}
		}
		else {
			BlImg = 55 - (1 + time) / 2;
			time--;
			if (time <= -10) {
				time = 0;
				BlImg = 50;
				MakeItem();
				Brown = true;
				Kill = 0;
				num = 1;
			}
		}
	}
	else {
		BlImg = 50;
	}
}

//////////////////////
//透明コインブロック//
//////////////////////
StealCoinBlock::StealCoinBlock() {
	Kill = 0;
	num = 13;
	img = 0;
	time = 0;
	BlImg = 80;
	alive = true;
}

void StealCoinBlock::MakeItem() {
	ObjectManager::GetInstance().MakeObject(1, pos.x, pos.y - 32);
	SoundManager::GetInstance().PlaySE(COIN);
}


////////////////////////////
//透明回復アイテムブロック//
////////////////////////////
StealHealingBlock::StealHealingBlock() {
	Kill = 0;
	num = 13;
	img = 0;
	time = 0;
	BlImg = 80;
	alive = true;
}

void StealHealingBlock::MakeItem() {
	SoundManager::GetInstance().PlaySE(COIN);
	ObjectManager::GetInstance().MakeObject(2, pos.x, pos.y - 32);
}


///////////////////////////
//透明1UPアイテムブロック//
///////////////////////////
StealOneUpBlock::StealOneUpBlock() {
	Kill = 0;
	num = 13;
	img = 0;
	time = 0;
	BlImg = 80;
	alive = true;
}

void StealOneUpBlock::MakeItem() {
	SoundManager::GetInstance().PlaySE(EXTEND);
	ObjectManager::GetInstance().MakeObject(3, pos.x, pos.y - 32);
}


//////////////////////
//透明ハシゴブロック//
//////////////////////
StealLadderMakerBlock::StealLadderMakerBlock() {
	Kill = 0;
	num = 13;
	img = 0;
	time = 0;
	BlImg = 80;
	alive = true;
}

void StealLadderMakerBlock::MakeItem() {
	SoundManager::GetInstance().PlaySE(LADDER);
	ObjectManager::GetInstance().MakeObject(4, pos.x, pos.y - 32);
}

//////////
//ハシゴ//
//////////

Ladder::Ladder() {
	Kill = 0;
	num = 10;
	img = 0;
	time = 0;
	BlImg = 24;
	alive = true;
}

void Ladder::update() {}

////////////////////////
//下から登れるブロック//
////////////////////////
Cloud::Cloud() {
	Kill = 0;
	num = 11;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void Cloud::update() {}


////////////////////
//降りれるブロック//
////////////////////
Through::Through() {
	Kill = 0;
	num = 12;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void Through::update() {}

////////////////
//土管ブロック//
////////////////
PipeUL::PipeUL() {
	Kill = 0;
	num = 14;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeUL::update() {}

PipeUR::PipeUR() {
	Kill = 0;
	num = 15;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeUR::update() {}

PipeDL::PipeDL() {
	Kill = 0;
	num = 16;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeDL::update() {}

PipeDR::PipeDR() {
	Kill = 0;
	num = 17;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeDR::update() {}

PipeLU::PipeLU() {
	Kill = 0;
	num = 18;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeLU::update() {}

PipeLD::PipeLD() {
	Kill = 0;
	num = 19;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeLD::update() {}

PipeRU::PipeRU() {
	Kill = 0;
	num = 20;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeRU::update() {}

PipeRD::PipeRD() {
	Kill = 0;
	num = 21;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void PipeRD::update() {}



//////////////////
//クリアアイテム//
//////////////////
BeatItem::BeatItem(){
	Kill = 0;
	num = 0;
	img = 0;
	time = 0;
	BlImg = 130;
	alive = true;
}

void BeatItem::update() {
	BlImg = 130 + time / 8;
	time++;
	if (time >= 80) {
		BlImg = 130;
		time = 0;
	}
}

void BeatItem::Touched() {
	if (alive) {
		PlayerManager::GetInstance().SetBeat(true);
		GameData::GetInstance().PlusScore(1000);
		alive = false;
	}
}

///////////////////
//クリアアイテム2//
///////////////////
BeatItem2::BeatItem2() {
	Kill = 0;
	num = 0;
	img = 0;
	time = 0;
	BlImg = 140;
	alive = true;
}

void BeatItem2::update() {
	BlImg = 140 + time / 8;
	time++;
	if (time >= 80) {
		BlImg = 140;
		time = 0;
	}
}

void BeatItem2::Touched() {
	if (alive) {
		PlayerManager::GetInstance().SetBeat(true);
		GameData::GetInstance().PlusScore(1000);
		alive = false;
	}
}

///////////////
//登る1/1坂道//
///////////////
SakaDRU::SakaDRU() {
	Kill = 0;
	num = 40;
	img =  0;
	time = 0;
	BlImg = 1;
	alive = true;
}

void SakaDRU::update() {}

/////////////////
//0枚なら通れる//
/////////////////
ZeroCoinBlock::ZeroCoinBlock() {
	Kill = 0;
	img = 0;
	time = 0;
	if (PlayerManager::GetInstance().GetCoin() == 0)num = 0;
	else num = 1;
	BlImg = 175 - num;
	alive = true;
}

void ZeroCoinBlock::update() {
	if (PlayerManager::GetInstance().GetCoin() == 0)num = 0;
	else num = 1;
	BlImg = 175 - num;
}

////////////////////
//50枚以上で通れる//
////////////////////
O50CoinBlock::O50CoinBlock() {
	Kill = 0;
	img = 0;
	time = 0;
	if (PlayerManager::GetInstance().GetCoin() >= 50)num = 0;
	else num = 1;
	BlImg = 171 - num;
	alive = true;
}

void O50CoinBlock::update() {
	if (PlayerManager::GetInstance().GetCoin() >= 50)num = 0;
	else num = 1;
	BlImg = 171 - num;
}

////////////////////
//50枚未満で通れる//
////////////////////
U50CoinBlock::U50CoinBlock() {
	Kill = 0;
	img = 0;
	time = 0;
	if (PlayerManager::GetInstance().GetCoin() < 50)num = 0;
	else num = 1;
	BlImg = 173 - num;
	alive = true;
}

void U50CoinBlock::update() {
	if (PlayerManager::GetInstance().GetCoin() < 50)num = 0;
	else num = 1;
	BlImg = 173 - num;
}

//////////////////////
//消えたり出たりする//
//////////////////////
DisAppBlock1::DisAppBlock1() {
	Kill = 0;
	img = 0;
	time = 0;
	num = 1;
	BlImg = 75;
	alive = true;
}

void DisAppBlock1::update() {
	int tmp = GameData::GetInstance().GetHiddenTime();
	if ((tmp % 80) == 0) {
		num = 1 - num;
		float Plyx = PlayerManager::GetInstance().GetPlayerX();
		if(AbsF(pos.x-Plyx) < WINDOWX*3/2)GameData::GetInstance().HiddenSE();
	}
	BlImg = 74 + num;
}

//////////////////////
//消えたり出たりする//
//////////////////////
DisAppBlock2::DisAppBlock2() {
	Kill = 0;
	img = 0;
	time = 0;
	num = 0;
	BlImg = 74;
	alive = true;
}

void DisAppBlock2::update() {
	int tmp = GameData::GetInstance().GetHiddenTime();
	if ((tmp % 80) == 0) {
		num = 1 - num;
		float Plyx = PlayerManager::GetInstance().GetPlayerX();
		if (AbsF(pos.x - Plyx) < WINDOWX * 3 / 2)GameData::GetInstance().HiddenSE();
	}
	BlImg = 74 + num;
}

/////////////////////////////
//ONOFFで消えたり出たりする//
/////////////////////////////
ONOFFDABlock1::ONOFFDABlock1() {
	Kill = 0;
	img = 0;
	time = 0;
	num = 0;
	BlImg = 71;
	alive = true;
}

void ONOFFDABlock1::update() {
	bool tmp = GameData::GetInstance().GetONOFF();
	if (tmp) {
		num = 1;
		BlImg = 71;
	} else {
		num = 0;
		BlImg = 70;
	}
}
ONOFFDABlock2::ONOFFDABlock2() {
	Kill = 0;
	img = 1;
	time = 0;
	num = 0;
	BlImg = 72;
	alive = true;
}
void ONOFFDABlock2::update() {
	bool tmp = GameData::GetInstance().GetONOFF();
	if (tmp) {
		num = 0;
		BlImg = 72;
	}
	else {
		num = 1;
		BlImg = 73;
	}
}

/////////////////
//ONOFFブロック//
/////////////////
ONOFF::ONOFF() {
	Kill = 0;
	img = 0;
	time = 0;
	num = 1;
	BlImg = 90;
	alive = true;
}

void ONOFF::update() {
	bool tmp = GameData::GetInstance().GetONOFF();
	if (tmp)BlImg = 90;
	else BlImg = 91;
	if (time > 0) {
		time++;
		BlImg = 95 + (time /2);
		if (time >= 10) {
			time = 0;
			BlImg = 91;
		}
	}
	if(time <0){
		time--;
		BlImg = 91 - (time / 2);
		if (time <= -10) {
			time = 0;
			BlImg = 90;
		}
	}
}

void ONOFF::Hited() {
	bool tmp = GameData::GetInstance().GetONOFF();
	if (tmp)time = -1;
	else time = 1;
	GameData::GetInstance().ChangeONOFF();
}

////////////////
//水中ブロック//
////////////////
WaterBlock::WaterBlock() {
	Kill = 0;
	num = 5;
	img = 0;
	time = 0;
	BlImg = 0;
	alive = true;
}

void WaterBlock::update() {}

//////////////////////
//上方向重力ブロック//
//////////////////////
UpGravBlock::UpGravBlock() {
	Kill = 0;
	num = 7;
	img = 0;
	time = 0;
	BlImg = 78;
	alive = true;
}

void UpGravBlock::update() {}

//////////////////////
//下方向重力ブロック//
//////////////////////
DownGravBlock::DownGravBlock() {
	Kill = 0;
	num = 8;
	img = 0;
	time = 0;
	BlImg = 77;
	alive = true;
}

void DownGravBlock::update() {}