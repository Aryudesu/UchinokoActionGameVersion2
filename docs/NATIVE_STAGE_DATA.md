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
- optional semantic terrain definitions (`TileDefinition`)

画像handleそのものはStageDataへ持たせず、DxLib依存はrenderer/runtime側へ隔離する。

## TileLayer

タイル単位で意味を持つもの。

- Terrain: Areaごとに正本を1枚
- Visual: 0枚以上。背景装飾、前景装飾等を任意に重ねられる
- ZOrderで描画順を表現

全TileLayerはAreaと同じgridサイズ/tileサイズを持つ。

Native JSONでは各TileLayerが `TileSetId` を参照する。

Visual LayerのCSV値は画像index。

Terrain LayerのCSV値は意味IDであり、TileSet内の `TerrainTiles` から、

```text
Terrain ID
  ├ Collision / Movement → CharacterController
  ├ Rules                → TileBehaviorSystem
  └ ImageIndex           → Renderer
```

へ分岐する。

`TileSetDefinition::BuildTerrainCatalog()` により既存Foundationの `TileCatalog` を生成するため、native専用の衝突・ギミック実装は作らない。

Native JSONの `terrainTiles[].rules` は既存 `TileRule` へそのまま変換する。CharacterControllerが生成した `TileInteraction` を `TileBehaviorSystem` に渡すことで、ReplaceTile / Coin / Damage等の既存Foundation behaviorを再利用する。

### Gameplay adapter pipeline

NativeStageSandboxはTileBehaviorSystemが返した意味effectを既存Foundation systemへ分配する。

```text
CharacterController::Interactions
            ↓
     TileBehaviorSystem
            ↓
       TileEffect[]
       ├ SpawnItem    → ItemSystem
       ├ BrickHit     → BrickSystem
       ├ ToggleSwitch → WorldState
       ├ resource     → PlayerResourceRules
       └ Goal         → GoalState
```

WorldStateでSolidが新規出現した時は `CharacterSafety::ResolveActivatedSolids()` を通すため、ON/OFF切替による挟み込み判定もNative専用実装を作らない。

ItemSystem / BrickSystemはArea固有runtimeとしてArea切替時にresetし、WorldStateのswitch値は同一Stage内のArea移動で保持する。

`native-test` では以下を一画面で確認できる。

- `?`: Healing SpawnItem。HP4 → HP5
- `B`: Brick。HP4ではbump、HP5ではbreak
- `S`: channel 0 toggle
- `W0`: channel 0へ束縛されたON/OFF block
- Coin / Pipe / Goal: 既存fixtureを継続

## ObjectLayer

Enemy、Lift、MovingPlatform、Item、PlayerSpawn等。

ObjectSpawnはworld座標を持つため、

- Terrainと同じ場所
- 複数Objectを同じ場所
- tile境界外

へ自由に配置できる。

### Native Object runtime / HitBounds

`ObjectSpawn` は配置データであり、実行時には `NativeObjectRuntime` へ変換する。

```text
ObjectSpawn
   ↓ ActivateArea
NativeObjectRuntime
 ├ Position
 ├ HitboxOffset
 ├ HitboxSize
 ├ ContactDamage
 └ Active
```

Playerとの通常接触は、見た目32x32全体ではなく既存 `CharacterController::TouchBounds()` の中央16x32を使う。

```text
Player TouchBounds (16x32)
        ×
Object HitBounds
        ↓
NativeObjectContact
```

Object側のHitBoundsはTypeIdごとの既定値を持つ。

- `WalkingEnemy`: offset=(8,1), size=(16,31)。V1 WalkingEnemy1の32x32 + gap.x=8 / gap.y=1由来
- `HorizontalLift`: offset=(-6,11), size=(44,10)。Native sandboxで従来debug表示していた足場形状
- その他TypeId: 当面32x32のdebug/runtime既定値

必要ならObject propertyで `hitboxOffset` / `hitboxSize` / `contactDamage` を上書きできる。

WalkingEnemyの `contactDamage` はSandboxで既存Foundation `DamageReactionState` へ接続する。

```text
Player TouchBounds
      ×
Enemy HitBounds
      ↓
contactDamage
      ↓
DamageReactionState
 ├ damage開始時にY速度reset
 ├ 1～15F: sourceと反対方向へ3px/frame
 └ 16F: reaction解除
```

Damage中は `DamageReactionState::Begin()` がfalseを返すため、同じEnemy・別Enemy・Damage terrainのいずれからも重複damageを受けない。16F解除後も危険源と接触していれば再度damage可能。#38のObject ID単位の「接触開始時のみ」抑制はdebug接触表示だけに変更し、無敵時間の責務をDamageReactionStateへ一本化する。

Terrainの `TileEffectType::Damage` も同じ経路へ通し、Tile hazardとEnemy contactで被ダメージ挙動を分けない。

踏みつけ、Enemy AI移動は後続で接続する。

LiftについてはHitBoundsをruntime化済みだが、Playerを乗せるStand/足元判定は後続実装とする。

## RegionLayer

Goal、Checkpoint、Camera/BGM trigger、Scene event等。

PointまたはRectangleで表現する。

### Goal Region runtime

`TypeId = "Goal"` のRegionは、CharacterBodyの矩形がRegion geometryへ実際に重なった時に発火する。

```text
StageRegionGeometry
      ↓ overlap
Goal Region
      ↓ goalKind = normal / secret
StageCompletionState
StageClearState
```

境界に触れただけでは発火せず、半開矩形として内部へ入った時に成立する。

Playerとの判定には見た目32x32の全身矩形ではなく、`CharacterController::TouchBounds()` の中央16x32を使う。これにより、見た目の端がGoalへ少し触れただけではクリアにならない。

`native-test` のGoal Regionは32x32。Region自体のサイズは固定せず、将来の大きいGoal/Triggerも同じ仕組みで表現できる。

## StageTransition

入口領域と移動先を直接持つ。

- 同一Area
- 別Area
- 別Stage

を同じモデルで表現する。

HSPのmov/coo/infやV1のStageMovingをruntimeへ持ち込まない。

### Pipe Transition runtime

`TypeId = "Pipe"` のStageTransitionは、NativeStageSandboxでは既存Foundation `PipeTransport` へ変換する。

```text
StageTransition
  Entry + EnterDirection
          ↓
      PipeTransport
          ↓ fade out
   TargetAreaへ切替
          ↓
  ExitPosition + ExitDirection
```

同一Stage内のArea移動では完全暗転時にTerrain/Layer/Regionの参照をTargetAreaへ切り替える。外部Stage遷移はStageロード責務が必要なため、このSandboxではまだ実行しない。

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
- ReplaceTile / BreakTileの移行先terrain IDが同一TileSet内に存在
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
