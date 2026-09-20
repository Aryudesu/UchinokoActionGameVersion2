#pragma once

#include "Foundation/CharacterController.h"
#include "Foundation/TileInteraction.h"
#include "Foundation/ItemSystem.h"
#include "Foundation/PipeTransport.h"
#include "Foundation/CharacterSafety.h"
#include "Foundation/ConditionalTerrain.h"
#include "Foundation/WorldState.h"
#include "Scene.h"

#include <string>
#include <vector>

class GimmickSandboxScene : public Scene {
public:
	GimmickSandboxScene();
	void update() override;
	void draw() override;

private:
	void Reload();
	void ApplyEffects();
	void ApplyEffectList(const std::vector<uchinoko::TileEffect>& Effects);
	uchinoko::GameStateSnapshot MakeGameStateSnapshot() const;
	void SynchronizeConditionalTerrain();

	uchinoko::TileMap Map_;
	uchinoko::TileCatalog Catalog_;
	uchinoko::TileRuntimeMap Runtime_;
	uchinoko::ItemSystem Items_;
	uchinoko::WorldState World_;
	std::vector<uchinoko::PipeLink> Pipes_;
	uchinoko::PipeTransport Pipe_;
	uchinoko::CharacterController Player_;
	std::string LoadError_;
	int Coins_ = 0;
	int Score_ = 0;
	int Health_ = 0;
	int Lives_ = 0;
	int Broken_ = 0;
	bool Dead_ = false;
};
