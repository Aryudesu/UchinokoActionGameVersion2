#include "LevelChanger.h"


void LevelChanger::Change(int n) { NowScene = n; };
int LevelChanger::GetLevelNum() { return NowScene; };
void LevelChanger::ResetLevel() { NowScene = NONES; };