#include "ScreenInfo.h"
#include "Singleton.h"
#include "Conf.h"
#include "DxLib.h"
#pragma once

void ScreenInfo::SetWorldSize(VECTOR Size) {
	WorldSize.x = Size.x;
	WorldSize.y = Size.y;
}

void ScreenInfo::CalcScreen(float PlayerX, float PlayerY) {
	if (PlayerY - 32. <= WorldSize.y * 32) {
		if (PlayerX + OBJSIZEX / 2 > WINDOWX / 2) { ScreenLeft = (PlayerX - WINDOWX / 2 + OBJSIZEX / 2); }
		else { ScreenLeft = 0; }
	}
}
