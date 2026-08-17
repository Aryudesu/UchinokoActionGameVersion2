#pragma once
#include "Singleton.h"
#include "Conf.h"
#include "DxLib.h"

class ScreenInfo : public Singleton<ScreenInfo> {
private:
	float ScreenLeft;
	float ScreenTop;
	VECTOR WorldSize;

public:
	void SetWorldSize(VECTOR Size);

	void CalcScreen(float PlayerX, float PlayerY);
};