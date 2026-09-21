#include "NativeStageSandboxScene.h"

#include "Conf.h"
#include "DxLib.h"
#include "Foundation/NativeStageDataLoader.h"
#include "InputKey.h"
#include "SceneChanger.h"

#include <algorithm>
#include <utility>

namespace {
constexpr int StageOffsetX = 64;
constexpr int StageOffsetY = 120;

int ScreenX(float WorldX) {
	return StageOffsetX + static_cast<int>(WorldX);
}

int ScreenY(float WorldY) {
	return StageOffsetY + static_cast<int>(WorldY);
}
}

NativeStageSandboxScene::NativeStageSandboxScene() {
	Reload();
}

NativeStageSandboxScene::~NativeStageSandboxScene() {
	DestroyTileSets();
}

void NativeStageSandboxScene::DestroyTileSets() {
	for (auto& Pair : TileSets_) {
		for (int Handle : Pair.second.Handles) {
			if (Handle >= 0) DeleteGraph(Handle);
		}
	}
	TileSets_.clear();
}

bool NativeStageSandboxScene::LoadTileSets() {
	SetTransColor(TRANSR, TRANSG, TRANSB);

	for (const uchinoko::TileSetDefinition& Definition : Stage_.TileSets) {
		LoadedTileSet Loaded;
		Loaded.EmptyTileId = Definition.EmptyTileId;
		Loaded.Transparent = Definition.Transparent;
		Loaded.Handles.assign(
			static_cast<std::size_t>(Definition.TileCount()), -1);

		const int Result = LoadDivGraph(
			Definition.ImageFile.c_str(),
			Definition.TileCount(),
			Definition.Columns,
			Definition.Rows,
			Definition.TileWidth,
			Definition.TileHeight,
			Loaded.Handles.data());
		if (Result == -1) {
			LoadError_ =
				"TileSet image load failed: " +
				Definition.Id + " -> " + Definition.ImageFile;
			DestroyTileSets();
			return false;
		}

		TileSets_.emplace(Definition.Id, std::move(Loaded));
	}
	return true;
}

void NativeStageSandboxScene::Reload() {
	DestroyTileSets();
	Area_ = nullptr;
	LoadError_.clear();

	uchinoko::Result<uchinoko::StageData> Loaded =
		uchinoko::NativeStageDataLoader::Load(
			"dat/stage/native-test/stage.json");
	if (Loaded.IsFailure()) {
		LoadError_ = Loaded.Error();
		return;
	}

	Stage_ = std::move(Loaded.Value());
	Area_ = Stage_.FindArea(Stage_.StartAreaId);
	if (Area_ == nullptr) {
		LoadError_ = "Start area not found: " + Stage_.StartAreaId;
		return;
	}

	LoadTileSets();
}

void NativeStageSandboxScene::update() {
	if (ReturnKey(KEY_INPUT_ESCAPE) == 1) {
		SceneChanger::GetInstance().Change(MENU);
		return;
	}
	if (ReturnKey(KEY_INPUT_R) == 1) Reload();
	if (ReturnKey(KEY_INPUT_D) == 1) ShowDebug_ = !ShowDebug_;
}

void NativeStageSandboxScene::DrawTileLayer(
	const uchinoko::TileLayer& Layer) {
	const auto Found = TileSets_.find(Layer.TileSetId);
	if (Found == TileSets_.end()) return;
	const LoadedTileSet& TileSet = Found->second;

	for (int Row = 0; Row < Layer.Map.Height(); ++Row) {
		for (int Column = 0; Column < Layer.Map.Width(); ++Column) {
			const int* TileId = Layer.Map.TryGet({Column, Row});
			if (TileId == nullptr || *TileId == TileSet.EmptyTileId) continue;

			const int Left =
				StageOffsetX + Column * Layer.Map.TileWidth();
			const int Top =
				StageOffsetY + Row * Layer.Map.TileHeight();

			if (*TileId < 0 ||
				static_cast<std::size_t>(*TileId) >= TileSet.Handles.size()) {
				DrawBox(
					Left, Top,
					Left + Layer.Map.TileWidth(),
					Top + Layer.Map.TileHeight(),
					GetColor(255, 60, 220), FALSE);
				DrawFormatString(
					Left + 3, Top + 8,
					GetColor(255, 255, 255),
					"%d", *TileId);
				continue;
			}

			DrawGraph(
				Left,
				Top,
				TileSet.Handles[static_cast<std::size_t>(*TileId)],
				TileSet.Transparent ? TRUE : FALSE);
		}
	}

	if (ShowDebug_) {
		DrawFormatString(
			StageOffsetX + Area_->Width * Area_->TileWidth + 24,
			StageOffsetY + 18 * Layer.Metadata.ZOrder / 10 + 60,
			GetColor(210, 220, 255),
			"z=%d  %s",
			Layer.Metadata.ZOrder,
			Layer.Metadata.Name.c_str());
	}
}

void NativeStageSandboxScene::DrawObjectLayer(
	const uchinoko::ObjectLayer& Layer) {
	for (const uchinoko::ObjectSpawn& Object : Layer.Objects) {
		const int X = ScreenX(Object.Position.X);
		const int Y = ScreenY(Object.Position.Y);

		if (Object.TypeId == "WalkingEnemy") {
			DrawBox(
				X + 2, Y + 2, X + 30, Y + 30,
				GetColor(240, 90, 90), FALSE);
			DrawString(X + 10, Y + 8, "E", GetColor(255, 170, 170));
		} else if (Object.TypeId == "HorizontalLift") {
			DrawBox(
				X - 6, Y + 11, X + 38, Y + 21,
				GetColor(90, 180, 255), FALSE);
			DrawString(X + 12, Y - 6, "L", GetColor(160, 220, 255));
		} else {
			DrawCircle(X + 16, Y + 16, 12, GetColor(120, 230, 160), FALSE);
		}

		if (ShowDebug_) {
			DrawFormatString(
				X, Y + 34, GetColor(255, 255, 255),
				"%s", Object.Id.c_str());
		}
	}
}

void NativeStageSandboxScene::DrawGeometry(
	const uchinoko::StageRegionGeometry& Geometry,
	unsigned int Color,
	const char* Label) {
	const int Left = ScreenX(Geometry.Position.X);
	const int Top = ScreenY(Geometry.Position.Y);

	if (Geometry.Shape == uchinoko::StageRegionShape::Point) {
		DrawCircle(Left, Top, 7, Color, FALSE);
		DrawLine(Left - 10, Top, Left + 10, Top, Color, 2);
		DrawLine(Left, Top - 10, Left, Top + 10, Color, 2);
	} else {
		const int Right = ScreenX(
			Geometry.Position.X + Geometry.Size.X);
		const int Bottom = ScreenY(
			Geometry.Position.Y + Geometry.Size.Y);
		DrawBox(Left, Top, Right, Bottom, Color, FALSE);
	}

	if (ShowDebug_ && Label != nullptr) {
		DrawString(Left + 4, Top + 4, Label, Color);
	}
}

void NativeStageSandboxScene::DrawRegionLayer(
	const uchinoko::RegionLayer& Layer) {
	for (const uchinoko::StageRegion& Region : Layer.Regions) {
		DrawGeometry(
			Region.Geometry,
			GetColor(80, 240, 210),
			Region.TypeId.c_str());
	}
}

void NativeStageSandboxScene::DrawTransitions() {
	if (Area_ == nullptr) return;
	for (const uchinoko::StageTransition& Transition : Area_->Transitions) {
		const unsigned int Color = GetColor(255, 210, 70);
		DrawGeometry(
			Transition.Entry,
			Color,
			Transition.TypeId.c_str());

		if (Transition.TargetStageId.empty() &&
			Transition.TargetAreaId == Area_->Id) {
			const int EntryX = ScreenX(Transition.Entry.Position.X);
			const int EntryY = ScreenY(Transition.Entry.Position.Y);
			const int ExitX = ScreenX(Transition.ExitPosition.X);
			const int ExitY = ScreenY(Transition.ExitPosition.Y);
			DrawLine(EntryX, EntryY, ExitX, ExitY, Color, 1);
			DrawCircle(ExitX, ExitY, 5, Color, TRUE);
		}
	}
}

void NativeStageSandboxScene::draw() {
	if (!LoadError_.empty()) {
		DrawString(
			16, 16,
			"Native stage load error (R: retry, Esc: menu)",
			GetColor(255, 100, 100));
		DrawString(
			16, 44,
			LoadError_.c_str(),
			GetColor(255, 255, 255));
		return;
	}
	if (Area_ == nullptr) return;

	std::vector<DrawLayerEntry> DrawOrder;
	int Order = 0;
	for (std::size_t Index = 0; Index < Area_->TileLayers.size(); ++Index) {
		if (!Area_->TileLayers[Index].Metadata.Visible) continue;
		DrawOrder.push_back({
			Area_->TileLayers[Index].Metadata.ZOrder,
			Order++,
			DrawLayerKind::Tile,
			Index});
	}
	for (std::size_t Index = 0; Index < Area_->ObjectLayers.size(); ++Index) {
		if (!Area_->ObjectLayers[Index].Metadata.Visible) continue;
		DrawOrder.push_back({
			Area_->ObjectLayers[Index].Metadata.ZOrder,
			Order++,
			DrawLayerKind::Object,
			Index});
	}
	for (std::size_t Index = 0; Index < Area_->RegionLayers.size(); ++Index) {
		if (!Area_->RegionLayers[Index].Metadata.Visible) continue;
		DrawOrder.push_back({
			Area_->RegionLayers[Index].Metadata.ZOrder,
			Order++,
			DrawLayerKind::Region,
			Index});
	}

	std::stable_sort(
		DrawOrder.begin(),
		DrawOrder.end(),
		[](const DrawLayerEntry& Left, const DrawLayerEntry& Right) {
			if (Left.ZOrder != Right.ZOrder) {
				return Left.ZOrder < Right.ZOrder;
			}
			return Left.Order < Right.Order;
		});

	for (const DrawLayerEntry& Entry : DrawOrder) {
		switch (Entry.Kind) {
		case DrawLayerKind::Tile:
			DrawTileLayer(Area_->TileLayers[Entry.Index]);
			break;
		case DrawLayerKind::Object:
			DrawObjectLayer(Area_->ObjectLayers[Entry.Index]);
			break;
		case DrawLayerKind::Region:
			DrawRegionLayer(Area_->RegionLayers[Entry.Index]);
			break;
		}
	}
	DrawTransitions();

	DrawString(
		16, 16,
		"Native Stage test: JSON + CSV + TileSet / R reload / D debug / Esc",
		GetColor(255, 255, 255));
	DrawFormatString(
		16, 42,
		GetColor(210, 230, 255),
		"Stage:%s  Area:%s  %dx%d tiles",
		Stage_.Id.c_str(),
		Area_->Id.c_str(),
		Area_->Width,
		Area_->Height);
	DrawString(
		16, 68,
		"Red=Enemy  Blue=Lift  Cyan=Region  Yellow=Transition",
		GetColor(230, 235, 255));
}
