#pragma once

#include "Foundation/BrickSystem.h"
#include "Foundation/Camera2D.h"
#include "Foundation/CharacterController.h"
#include "Foundation/DamageReactionState.h"
#include "Foundation/GoalState.h"
#include "Foundation/ItemSystem.h"
#include "Foundation/NativeObjectRuntime.h"
#include "Foundation/PipeTransport.h"
#include "Foundation/StageData.h"
#include "Foundation/WorldState.h"
#include "Scene.h"

#include <string>
#include <unordered_map>
#include <vector>

class NativeStageSandboxScene : public Scene {
public:
	NativeStageSandboxScene();
	~NativeStageSandboxScene() override;

	void update() override;
	void draw() override;

private:
	struct LoadedTileSet {
		int EmptyTileId = 0;
		bool Transparent = true;
		std::vector<int> Handles;
	};

	enum class DrawLayerKind {
		Tile,
		Object,
		RuntimeEffects,
		Player,
		Region
	};

	struct DrawLayerEntry {
		int ZOrder = 0;
		int Order = 0;
		DrawLayerKind Kind = DrawLayerKind::Tile;
		std::size_t Index = 0;
	};

	void Reload();
	void DestroyTileSets();
	bool LoadTileSets();
	bool ActivateArea(const std::string& AreaId);
	bool InitializeNativePlayer();
	int RequiredSwitchCount() const;
	uchinoko::GameStateSnapshot MakeGameStateSnapshot() const;
	void ApplyEffectList(const std::vector<uchinoko::TileEffect>& Effects);
	bool BeginPlayerDamage(int Damage, float SourceCenterX);
	void ApplyTerrainEffects();
	void ApplyObjectContacts();
	void CheckGoalRegions();
	bool TryBeginTransition(const uchinoko::CharacterInput& Input);
	void UpdateTransition();
	void DrawTileLayer(const uchinoko::TileLayer& Layer);
	void DrawRuntimeEffects();
	void DrawPlayer();
	void DrawObjectLayer(const uchinoko::ObjectLayer& Layer);
	void DrawRegionLayer(const uchinoko::RegionLayer& Layer);
	void DrawTransitions();
	void UpdateCamera();
	int ScreenX(float WorldX) const;
	int ScreenY(float WorldY) const;
	void DrawGeometry(
		const uchinoko::StageRegionGeometry& Geometry,
		unsigned int Color,
		const char* Label);

	uchinoko::StageData Stage_;
	uchinoko::StageArea* Area_ = nullptr;
	uchinoko::TileLayer* TerrainLayer_ = nullptr;
	uchinoko::TileCatalog TerrainCatalog_;
	uchinoko::TileRuntimeMap TerrainRuntime_;
	uchinoko::CharacterController Player_;
	uchinoko::Camera2D Camera_;
	uchinoko::PlatformerCameraSettings CameraSettings_;
	uchinoko::ItemSystem Items_;
	uchinoko::BrickSystem Bricks_;
	uchinoko::WorldState World_;
	uchinoko::NativeObjectSystem Objects_;
	uchinoko::DamageReactionState DamageReaction_;
	uchinoko::PipeTransport Pipe_;
	uchinoko::StageCompletionState Completion_;
	uchinoko::StageClearState ClearState_;
	std::string ActiveTransitionId_;
	std::string ActiveTransitionTargetAreaId_;
	std::vector<std::string> ActiveObjectContacts_;
	bool PlayerReady_ = false;
	int Coins_ = 0;
	int Score_ = 0;
	int Health_ = 4;
	int Lives_ = 3;
	int Broken_ = 0;
	int LadderTileId_ = -1;
	int FacingDirection_ = 1;
	bool Dead_ = false;
	std::unordered_map<std::string, LoadedTileSet> TileSets_;
	std::string LoadError_;
	bool ShowDebug_ = true;
};
