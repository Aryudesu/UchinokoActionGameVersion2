#include "BlockFactory.h"


Block * BlockFactory::BlkFactory(int ID) {
	if (ID == 0)return new Empty();
	if (ID == 1)return new Steals();
	if (ID == 2)return new Bricks();
	if (ID == 3)return new Coin();
	if (ID == 4)return new CoinBlock();
	if (ID == 5)return new Coin10Block();
	if (ID == 6)return new HealingBlock();
	if (ID == 7)return new OneUpBlock();
	if (ID == 8)return new LadderMakerBlock();
	if (ID == 9)return new BrownBlock();
	if (ID == 10)return new Ladder();
	if (ID == 11)return new Cloud();
	if (ID == 12)return new Through();
	if (ID == 13)return new PipeUL();
	if (ID == 14)return new PipeUR();
	if (ID == 15)return new PipeDL();
	if (ID == 16)return new PipeDR();
	if (ID == 17)return new PipeLU();
	if (ID == 18)return new PipeLD();
	if (ID == 19)return new PipeRU();
	if (ID == 20)return new PipeRD();
	if (ID == 21)return new ONOFF();
	if (ID == 22)return new WaterBlock();
	if (ID == 23)return new HealingCoin();
	if (ID == 24)return new StealCoinBlock();
	if (ID == 25)return new OneUPCoin();
	if (ID == 26)return new StealHealingBlock();
	if (ID == 27)return new StealOneUpBlock();
	if (ID == 28)return new StealLadderMakerBlock();
	if (ID == 29)return new ZeroCoinBlock();
	if (ID == 30)return new BeatItem();
	if (ID == 31)return new BeatItem2();
	if (ID == 32)return new UpGravBlock();
	if (ID == 33)return new DownGravBlock();
	if (ID == 34)return new DisAppBlock1();
	if (ID == 35)return new DisAppBlock2();
	if (ID == 36)return new O50CoinBlock();
	if (ID == 37)return new U50CoinBlock();
	if (ID == 38)return new ONOFFDABlock1();
	if (ID == 39)return new ONOFFDABlock2();
	if (ID >= 40 && ID <= 45) {
		Block *tmp = new Steals();
		tmp->SetKill(ID-39);
		return tmp;
	}
	return new Empty();
}