#pragma once

#include "Foundation/CharacterController.h"
#include "Foundation/StageData.h"
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
	bool InitializeNativePlayer();
	void DrawTileLayer(const uchinoko::TileLayer& Layer);
	void DrawPlayer();
	void DrawObjectLayer(const uchinoko::ObjectLayer& Layer);
	void DrawRegionLayer(const uchinoko::RegionLayer& Layer);
	void DrawTransitions();
	void DrawGeometry(
		const uchinoko::StageRegionGeometry& Geometry,
		unsigned int Color,
		const char* Label);

	uchinoko::StageData Stage_;
	const uchinoko::StageArea* Area_ = nullptr;
	const uchinoko::TileLayer* TerrainLayer_ = nullptr;
	uchinoko::TileCatalog TerrainCatalog_;
	uchinoko::CharacterController Player_;
	bool PlayerReady_ = false;
	std::unordered_map<std::string, LoadedTileSet> TileSets_;
	std::string LoadError_;
	bool ShowDebug_ = true;
};
