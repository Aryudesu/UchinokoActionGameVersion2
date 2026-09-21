# Native Stage Data Model

Version2の本番runtimeと将来のステージエディタが共有する、旧形式に依存しないステージモデル。

## 方針

旧HSP / Version1の1セル1値構造を引き継がない。

```text
StageData
  ├ TileSets[]
  └ Areas[]
      ├ TileLayers[]
      ├ ObjectLayers[]
      ├ RegionLayers[]
      └ Transitions[]
```

保存形式はモデルとは分離する。最初のnative serializer形式としてJSON + CSVを採用し、仕様は `NATIVE_STAGE_FORMAT.md` に記録する。

## TileSet

TileLayerの描画元画像を定義する。

- stable ID
- image file
- tile pixel size
- source image grid (columns / rows)
- empty tile value
- transparency

画像handleそのものはStageDataへ持たせず、DxLib依存はrenderer/runtime側へ隔離する。

## TileLayer

タイル単位で意味を持つもの。

- Terrain: Areaごとに正本を1枚
- Visual: 0枚以上。背景装飾、前景装飾等を任意に重ねられる
- ZOrderで描画順を表現

全TileLayerはAreaと同じgridサイズ/tileサイズを持つ。

Native JSONでは各TileLayerが `TileSetId` を参照する。現在のSandbox描画ではCSV値をTileSet内の画像indexとして直接描画する。Terrainの意味ID→ImageIndex対応は、native gameplay runtime接続時にTileCatalogと統合する。

## ObjectLayer

Enemy、Lift、MovingPlatform、Item等。

ObjectSpawnはworld座標を持つため、

- Terrainと同じ場所
- 複数Objectを同じ場所
- tile境界外

へ自由に配置できる。

## RegionLayer

Goal、Checkpoint、Camera/BGM trigger、Scene event等。

PointまたはRectangleで表現する。

## StageTransition

入口領域と移動先を直接持つ。

- 同一Area
- 別Area
- 別Stage

を同じモデルで表現する。

HSPのmov/coo/infやV1のStageMovingをruntimeへ持ち込まない。

## Stable IDs

Layer / Object / Region / Transitionは文字列IDを持つ。

表示名とは分離し、エディタで名前を変更しても参照関係を維持できるようにする。

Area内ではLayer ID、およびObject/Region/Transitionのentity IDを一意にする。

## Properties

追加パラメータは型付きpropertyとして保持する。

対応型:

- Integer
- Float
- Boolean
- String
- Vector2

C++14のためstd::variantには依存しない。

例:

```text
Enemy
  direction = "left"

Lift
  range = 192.0
  speed = 2.0
```

将来のeditor schema側で、TypeIdごとにproperty名・型・enum候補・範囲等を定義できる。

## Validation

`ValidateStageData` が保存・読込境界で構造を検証する。

主な条件:

- Stage/Area/Layer/entity IDが空でない
- Area IDが一意
- StartAreaが存在
- TileSet IDが一意で、画像・分割サイズが妥当
- TileLayerが指定したTileSetが存在
- TileSetとAreaのtile sizeが一致
- AreaごとにTerrain layerがちょうど1枚
- TileLayerのgrid/tile sizeがAreaと一致
- Layer IDがArea内で一意
- Object/Region/Transition IDがArea内で一意
- Rectangle regionの幅・高さが正
- same-stage transitionのTargetAreaが存在

編集中は一時的に不完全な状態を許し、保存/export時にValidateする想定。

## Legacyとの関係

`LayeredMap` はV1資産を分解するmigration intermediateとして残す。

最終的には、

```text
V1 map/img/Data.inf
      ↓
migration parser
      ↓
LayeredMap / legacy metadata
      ↓
converter
      ↓
StageData
      ↓
native serializer
      ↓
V2 runtime
```

とする。

本番runtimeが旧形式を直接読むことは目標にしない。


## Future: Scrolling / Parallax Tile Layers

Some stages may need tile layers whose visual position moves differently from the main world/camera scroll.

Examples:

- distant background clouds
- parallax scenery
- foreground decoration
- screen-relative decorative layers

This should not become a mandatory stage feature.

The preferred extension is to add optional transform/scroll metadata to individual `TileLayer` / `LayerMetadata`, rather than adding one dedicated "scroll layer" field to `StageData`.

Conceptually:

```cpp
struct LayerTransform {
    WorldPosition Offset;
    WorldPosition ScrollFactor { 1.0f, 1.0f };
};
```

Typical meanings:

```text
(1.0, 1.0) normal world layer
(0.5, 1.0) horizontal parallax
(0.0, 0.0) camera-fixed visual
(1.2, 1.0) foreground moving faster than the camera
```

The current PR intentionally does not implement this yet.

### Moving collision terrain

Visual parallax and collision terrain movement are different problems.

PR #30 currently requires exactly one `Terrain` TileLayer per Area.

If a future stage needs an entire collision tile layer to move independently, this invariant may need to be relaxed or a separate moving-terrain concept introduced.

Before doing that, compare the requirement with `ObjectSpawn`-based moving platforms/lifts. Small or local moving terrain is often better represented as an object; a whole independently moving tile field may justify a dedicated moving tile layer.

Therefore:

- visual scrolling/parallax: planned as an optional per-layer property
- independently moving collision tile layers: deferred design decision
- existing stages do not need either feature
