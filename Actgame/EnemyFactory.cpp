#include "EnemyFactory.h"
#include "Object.h"
#include "Enemy.h"

Charactor* EnemyFactory::Factory(int ID, double initx, double inity) {
	if (ID == 1)return new WalkingEnemy1(initx,inity);
	if (ID == 2)return new WalkingEnemy2(initx, inity);
	if (ID == 3)return new CarrotMan(initx, inity);
	if (ID == 4)return new BallSlime(initx, inity);
	if (ID == 5)return new BallSlime2(initx, inity);
	return NULL;
}