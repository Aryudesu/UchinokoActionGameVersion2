#pragma once
#include "Map.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "Singleton.h"
//#include "Player.h"
#include <vector>

class ObjectManager : public Singleton<ObjectManager>{
private:
	std::vector<Object*> Obj;
	std::vector<Charactor*> Enemy;		//当たり判定計算しやすいかなって思って・・・
	std::vector<Object*> Effe;

	bool CollAll(Object* ObjA,Object* ObjB);
public:

	void CollPlyObj();
	void CollisionObj();
	void MakeObject(int num, int x, int y);
	void MakeEnemy(int num, int x, int y);
	void MakeEffect(int num, int x, int y);
	void MakeEffectObj(int num, int x, int y);
	void MakeScoreEffect(int num, int x, int y);
	void MakeBlockFragment(int num, int x, int y);
	void update(Map *M);
	void draw(Map *M);
	void ObjDel();

	void DeleteAll();

	int GetObjNum() { return Obj.size(); }
	int GetEnemyNum() { return Enemy.size(); }

	void KillAll(Map *M);
};