#include "SceneChanger.h"

void SceneChanger::Change(int n) { NowScene = n; };
int SceneChanger::GetSceneNum() { return NowScene; };
void SceneChanger::ResetScene() { NowScene = STEADY; };