# Native Stage Data Model

Version2の本番runtimeと将来のステージエディタが共有する、旧形式に依存しないステージモデル。

## 方針

旧HSP / Version1の1セル1値構造を引き継がない。

```text
StageData
  └ Areas[]
      ├ TileLayers[]
      ├ ObjectLayers[]
      ├ RegionLayers[]
      └ Transitions[]
```

保存形式(JSON等)はこのモデルとは分離し、後から決める。

## TileLayer

タイル単位で意味を持つもの。

- Terrain: Areaごとに正本を1枚
- Visual: 0枚以上。背景装飾、前景装飾等を任意に重ねられる
- ZOrderで描画順を表現

全TileLayerはAreaと同じgridサイズ/tileサイズを持つ。

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
