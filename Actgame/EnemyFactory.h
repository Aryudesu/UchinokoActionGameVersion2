#pragma once
#include "Object.h"
#include "Enemy.h"


class EnemyFactory : public Singleton<EnemyFactory> {
public:
	Charactor * Factory(int ID, double initx, double inity);
};