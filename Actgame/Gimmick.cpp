#include "Gimmick.h"
#include "Conf.h"
#include "ImageManager.h"


LadderMaker::LadderMaker(int x, int y) {
	initpos.x = pos.x = (float)x;
	initpos.y = pos.y = (float)y;
	dire = -1;
	Vmax.x = 10.0;
	Vmax.y = 12.0;
	speed.x = 0.0;
	speed.y = -4.0;
	gap.x = 8.0;
	gap.y = 0.0;
	HP = 1;
	dire = 1;
	alive = true;
	OnField = true;
}

void LadderMaker::UpdateSpeedX() {}

void LadderMaker::UpdateSpeedY() {}

void LadderMaker::MoveX(Map *M) {}

void LadderMaker::MoveY(Map *M) {
	if ((int)pos.y % MAPSIZEY <= abs(1.5*(int)speed.y)) {
		M->SetBlock(10, CenterXMap(), CenterYMap());
	}
	pos.y += speed.y;
	if (MapHitLU(M) == 1 || MapHitRU(M) == 1) {
		alive = false;
	}
}

void LadderMaker::update(Map *M) {
	MoveY(M);
}

void LadderMaker::animation() {
}

void LadderMaker::draw(Map *M) {
	ImageManager::GetInstance().DrawImg(pos.x - M->GetScreenLUX(), pos.y - M->GetScreenLUY(), OBJECT, 100, TRUE);
}
