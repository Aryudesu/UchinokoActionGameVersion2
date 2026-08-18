#include "ObjectFactory.h"
#include "Gimmick.h"
#include "Item.h"
#include "function.h"

Object* ObjectFactory::Factory(int ID, double initx, double inity) {
	if (ID == 1)return new CoinItem(initx , inity);
	if (ID == 2)return new HealingItem(initx, inity);
	if (ID == 3)return new OneUpItem(initx, inity);
	if (ID == 4)return new LadderMaker(initx, inity);
	return NULL;
}

Object* EffectFactory::Factory(int ID,double initx,double inity){
	return new Effect(ID,initx,inity);
}

Object* EffectObjFactory::Factory(int ID, double initx, double inity) {
	if (ID == 0)return new EffectObj(ID, initx, inity);
}

Object* ScoreEffectFactory::Factory(int ID,double initx, double inity) {
	return new ScoreEffect(ID,initx, inity);
}

Object* BlockFragmentFactory::Factory(int ID, double initx, double inity) {
	return new BlockFragment(ID, initx, inity);
}
