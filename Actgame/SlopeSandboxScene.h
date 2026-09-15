#pragma once

#include "Foundation/CharacterController.h"
#include "Scene.h"

class SlopeSandboxScene : public Scene {
public:
	SlopeSandboxScene();
	void update() override;
	void draw() override;

private:
	uchinoko::TileMap Map_;
	uchinoko::TileCatalog Catalog_;
	uchinoko::CharacterController Player_;
};
