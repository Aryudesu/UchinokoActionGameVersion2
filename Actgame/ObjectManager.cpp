#include "ObjectManager.h"
#include "ObjectFactory.h"
#include "EnemyFactory.h"
#include "PlayerManager.h"
#include "Conf.h"
#include "Map.h"
#include "function.h"

void ObjectManager::MakeObject(int num, int x, int y) {
	Obj.push_back(ObjectFactory::GetInstance().Factory(num,x,y));
}

void ObjectManager::MakeEnemy(int num, int x, int y) {
	Enemy.push_back(EnemyFactory::GetInstance().Factory(num, x, y));
}

void ObjectManager::MakeEffect(int num, int x, int y) {
	Effe.push_back(EffectFactory::GetInstance().Factory(num, x, y));
}

void ObjectManager::MakeEffectObj(int num, int x, int y) {
	Effe.push_back(EffectObjFactory::GetInstance().Factory(num, x, y));
}

void ObjectManager::MakeScoreEffect(int num, int x, int y) {
	Effe.push_back(ScoreEffectFactory::GetInstance().Factory(num, x, y));
}

void ObjectManager::MakeBlockFragment(int num, int x, int y) {
	Effe.push_back(BlockFragmentFactory::GetInstance().Factory(num, x, y));
}

void ObjectManager::update(Map *M) {
	for (Object* x : Obj) { x->update(M); }
	for (Object* x : Enemy) { x->update(M); }
	for (Object* x : Effe) { x->update(M); }
	ObjDel();
}

void ObjectManager::draw(Map *M) {
	for (Object* x: Obj) { x->draw(M); }
	for (Object* x: Enemy) { x->draw(M); }
	for (Object* x : Effe) { x->draw(M); }
}

void ObjectManager::ObjDel() {
	auto itr = Obj.begin();
	while (itr != Obj.end()) {
		if (!((*itr)->GetAlive())) {
			itr = Obj.erase(itr);
		} else {
			itr++;
		}
	}
	auto itr2 = Enemy.begin();
	while (itr2 != Enemy.end()) {
		if (!((*itr2)->GetAlive())) {
			itr2 = Enemy.erase(itr2);
		}
		else {
			itr2++;
		}
	}
	auto itr3 = Effe.begin();
	while (itr3 != Effe.end()) {
		if (!((*itr3)->GetAlive())) {
			itr3 = Effe.erase(itr3);
		}
		else {
			itr3++;
		}
	}
	return;
}

bool RectAColl(float x1,float y1,float w1,float h1,float x2,float y2,float w2,float h2) {
	if (AbsF((x1 * 2. + w1) - (x2 * 2. + w2)) < w1 + w2 && AbsF((y1 * 2. + h1) - (y2 * 2. + h2)) < h1 + h2)return true;
	return false;
}


bool ObjectManager::CollAll(Object* ObjA, Object* ObjB) {
	return RectAColl(ObjA->getX()+ObjA->getgapX(), 
					ObjA->getY() + ObjA->getgapY(), 
					OBJSIZEX - 2.*ObjA->getgapX(),
					OBJSIZEY - ObjA->getgapY(),
					ObjB->getX() + ObjB->getgapX(),
					ObjB->getY() + ObjB->getgapY(),
					OBJSIZEX - 2.*ObjB->getgapX(),
					OBJSIZEY - ObjB->getgapY()
					);
}


void ObjectManager::CollisionObj() {
	for (int i = 0; i < Enemy.size() - 1; i++) {
		for (int j = i + 1; j < Enemy.size(); j++) {
			if (Enemy[i]->GetOnField() && Enemy[j]->GetOnField()) {
				CollAll(Enemy[i], Enemy[j]);
			}
		}
	}
	return;
}

bool RectPCColl(Object *ObjA) {
	return RectAColl(ObjA->getX() + ObjA->getgapX(), ObjA->getY() + ObjA->getgapY(), OBJSIZEX - 2.*ObjA->getgapX(), OBJSIZEY - ObjA->getgapY(),
		PlayerManager::GetInstance().GetPlayerX() + PlayerManager::GetInstance().GetGapX(),
		PlayerManager::GetInstance().GetPlayerY() + PlayerManager::GetInstance().GetGapY(),
		OBJSIZEX - 2.*PlayerManager::GetInstance().GetGapX(),
		OBJSIZEY - PlayerManager::GetInstance().GetGapY()
	);
}

void ObjectManager::CollPlyObj() {
	for (int i = 0; i < Enemy.size(); i++) {
		//主人公との衝突
		//敵が画面内にいる時
		if (Enemy[i]->GetOnField() && RectPCColl(Enemy[i])) {
			//踏んだ時
			if (Enemy[i]->getY() - PlayerManager::GetInstance().GetPlayerY() <= 32 && Enemy[i]->getY() - PlayerManager::GetInstance().GetPlayerY() >= 8 && (PlayerManager::GetInstance().GetGravDire() == 1)) {
				//踏める敵だった時
				if (Enemy[i]->GetPress() == 1) {
					PlayerManager::GetInstance().Tread();
					Enemy[i]->Treaded();
					PlayerManager::GetInstance().PlusCombo();
				}
				else if(Enemy[i]->GetPress() == 2){
					//踏めない敵だった時
					Enemy[i]->ColliSide2ObjP(PlayerManager::GetInstance().GetPlayerX(), PlayerManager::GetInstance().GetPlayerY());
					PlayerManager::GetInstance().Damaged((Enemy[i]->getX() > PlayerManager::GetInstance().GetPlayerX()) ? -1 : 1);
				}
			} else {
				if (Enemy[i]->GetPDecision() == 1) {
					//踏んでない時
					Enemy[i]->ColliSide2ObjP(PlayerManager::GetInstance().GetPlayerX(), PlayerManager::GetInstance().GetPlayerY());
					PlayerManager::GetInstance().Damaged((Enemy[i]->getX() > PlayerManager::GetInstance().GetPlayerX()) ? -1 : 1);
				}
				if (Enemy[i]->GetPDecision() == 2) {
					Enemy[i]->ColliSide2ObjP(PlayerManager::GetInstance().GetPlayerX(), PlayerManager::GetInstance().GetPlayerY());
				}
			}
		}
		//敵同士の衝突
		if (Enemy[i]->GetOnField()) {
			for (int j = i; j < Enemy.size(); j++) {
				if (i != j && Enemy[j]->GetOnField() && CollAll(Enemy[i], Enemy[j]) && Enemy[i]->GetEDecision() != 0 && Enemy[j]->GetEDecision() != 0) {
					if (Enemy[j]->GetEDecision() == 2)Enemy[i]->ColliSide2Obj(Enemy[j]->getX(), Enemy[j]->getY());	//当たり判定がある時
					if (Enemy[i]->GetEDecision() == 2)Enemy[j]->ColliSide2Obj(Enemy[i]->getX(), Enemy[i]->getY());	//当たり判定がある時
					if (Enemy[j]->GetEDecision() == 1) {
						Enemy[i]->GetDamage(Enemy[j]->GetCombo());	//当たり判定がある時
						Enemy[j]->PlusCombo();
					}
					if (Enemy[i]->GetEDecision() == 1) {
						Enemy[j]->GetDamage(Enemy[i]->GetCombo());	//当たり判定がある時
						Enemy[i]->PlusCombo();
					}
				}
			}
		}
	}
}

void ObjectManager::DeleteAll() {
	Obj.clear();
	Enemy.clear();
	Effe.clear();
}

void ObjectManager::KillAll(Map *M) {
	int C = 0;
	for (int i = 0; i < Enemy.size(); i++) {
		if (Enemy[i]->InScreen(M) && Enemy[i]->GetOnField() && Enemy[i]->GetAlive()) {
			Enemy[i]->NaturalKilled(C);
			C++;
		} else {
			Enemy[i]->Delete();
		}
	}
}