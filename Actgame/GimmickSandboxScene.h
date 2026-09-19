#pragma once

#include "Foundation/CharacterController.h"
#include "Foundation/TileInteraction.h"
#include "Scene.h"

#include <string>

class GimmickSandboxScene : public Scene {
public:
	GimmickSandboxScene();
	void update() override;
	void draw() override;

private:
	void Reload();
	void ApplyEffects();

	uchinoko::TileMap Map_;
	uchinoko::TileCatalog Catalog_;
	uchinoko::TileRuntimeMap Runtime_;
	uchinoko::CharacterController Player_;
	std::string LoadError_;
	int Coins_ = 0;
	int Score_ = 0;
	int Broken_ = 0;
};
