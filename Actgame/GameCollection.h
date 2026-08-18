#pragma once
#include "Scene.h"
#include <vector>
#include <string>

class GameCollection{
protected:

public:
	GameCollection();
	virtual int GetBeatLevel() = 0;
	virtual bool update() = 0;
	virtual void draw() = 0;
};