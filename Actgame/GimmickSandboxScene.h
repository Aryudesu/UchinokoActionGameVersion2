#pragma once

#include "Foundation/CharacterController.h"
#include "Foundation/TileInteraction.h"
#include "Foundation/ItemSystem.h"
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
	void ApplyEffectList(const std::vector<uchinoko::TileEffect>& Effects);

	uchinoko::TileMap Map_;
	uchinoko::TileCatalog Catalog_;
	uchinoko::TileRuntimeMap Runtime_;
	uchinoko::ItemSystem Items_;
	uchinoko::CharacterController Player_;
	std::string LoadError_;
	int Coins_ = 0;
	int Score_ = 0;
	int Health_ = 0;
	int Lives_ = 0;
	int Broken_ = 0;
};
