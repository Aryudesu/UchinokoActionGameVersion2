#pragma once

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
	void DrawTileLayer(const uchinoko::TileLayer& Layer);
	void DrawObjectLayer(const uchinoko::ObjectLayer& Layer);
	void DrawRegionLayer(const uchinoko::RegionLayer& Layer);
	void DrawTransitions();
	void DrawGeometry(
		const uchinoko::StageRegionGeometry& Geometry,
		unsigned int Color,
		const char* Label);

	uchinoko::StageData Stage_;
	const uchinoko::StageArea* Area_ = nullptr;
	std::unordered_map<std::string, LoadedTileSet> TileSets_;
	std::string LoadError_;
	bool ShowDebug_ = true;
};
