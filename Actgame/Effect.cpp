#include "Object.h"
#include "ImageManager.h"
#include "function.h"
#include "Conf.h"
#include "Variable.h"


Effect::Effect() {
	OnField = true;
}

Effect::Effect(int ID,int x,int y) {
	Count = 0;
	BaseImg = ID * 10 + 9;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 0.0;
	Vmax.y = 0.0;
	speed.x = 0.0;
	speed.y = 0.0;
	alive = true;
	OnField = true;
	Tobiori = false;
}

void Effect::update(Map *M) {
	Count++;
	if (Count >= 40)alive = false;
}

void Effect::draw(Map *M) { ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), EFFECT, BaseImg - Count/4, TRUE); }



ScoreEffect::ScoreEffect() {
	OnField = true;
}

ScoreEffect::ScoreEffect(int ID,int x, int y) {
	Count = 0;
	if (ID<=9)BaseImg = 200 + ID;
	else BaseImg = 209;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 0.0;
	Vmax.y = 0.0;
	speed.x = 0.0;
	speed.y = 0.0;
	alive = true;
	OnField = true;
	Tobiori = false;
}

BlockFragment::BlockFragment() {
	OnField = true;
}

BlockFragment::BlockFragment(int ID, int x, int y) {
	Count = 0;
	BaseImg = ID + 10 * 21;
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	Vmax.x = 5.0;
	Vmax.y = 10.0;
	speed.x = rand()%11 - 5;
	speed.y = -rand()%15;
	alive = true;
	OnField = true;
	Tobiori = false;
}

void BlockFragment::update(Map *M) {
	speed.y += GRAVITY;
	if (Vmax.y < speed.y)speed.y = Vmax.y;
	pos.x += speed.x;
	pos.y += speed.y;
	if (pos.y > M->GetSLU().y + WINDOWY + OBJSIZEY * 3)alive = false;
}

void BlockFragment::draw(Map* M) { ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), EFFECT, BaseImg, TRUE); }

void ScoreEffect::update(Map *M) {
	Count++;
	if (Count >= 60)alive = false;
}

void ScoreEffect::draw(Map *M) { ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY() - Count, EFFECT, BaseImg, TRUE); }

EffectObj::EffectObj() {
	Count = 0;
	BaseImg = 0;
	initpos.x = pos.x = 0;
	initpos.y = pos.y = 0;
	Vmax.x = 0.0;
	Vmax.y = 0.0;
	speed.x = 0.0;
	speed.y = 0.0;
	alive = true;
	OnField = true;
	Tobiori = false;
}
EffectObj::EffectObj(int ID, int x, int y) {
	Count = 0;
	BaseImg = ID;
	initpos.x = pos.x = (float)x + (GetRand(11) - 5);
	initpos.y = pos.y = (float)y;
	Vmax.x =  4.0;
	Vmax.y = 12.0;
	speed.x = GetRand(70)/10. - 3;
	speed.y = -(GetRand(70)/10. + 3);
	alive = true;
	OnField = true;
	Tobiori = false;
}
void EffectObj::update(Map* M) {
	pos.x += speed.x;
	speed.y += GRAVITY;
	pos.y += speed.y;
	if (M->GetNum(pos.x/MAPSIZEX, pos.y/MAPSIZEY) != 0)alive = false;
}
void EffectObj::draw(Map* M) {
	if (BaseImg == 0)DrawBox(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), pos.x - M->GetScreenLUX() + 2, pos.y - M->GetScreenLUY() + 2, GetColor(255, 255, 255), TRUE);
}