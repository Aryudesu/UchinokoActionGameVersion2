#pragma once

#include "Foundation/CharacterController.h"
#include "Scene.h"

#include <string>

class SlopeSandboxScene : public Scene {
public:
	SlopeSandboxScene();
	void update() override;
	void draw() override;

private:
	void Reload();

	uchinoko::TileMap Map_;
	uchinoko::TileCatalog Catalog_;
	uchinoko::CharacterController Player_;
	std::string LoadError_;
};
