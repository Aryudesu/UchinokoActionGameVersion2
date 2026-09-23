# Native Stage File Format v1

V2 native `StageData` の最初の保存形式。

- 構造化データ: JSON
- TileLayer grid: CSV
- JSON内の `source` は `stage.json` からの相対パス

## Dependency

JSON parsing uses `nlohmann/json` v3.12.0.

To keep the existing Visual Studio project self-contained, the official single-header distribution is vendored at:

```text
third_party/nlohmann/json.hpp
```

The upstream MIT license is kept at:

```text
third_party/nlohmann/LICENSE.MIT
```

Both Actgame and FoundationTests add `$(SolutionDir)third_party` to their include path, so no vcpkg installation or global Visual Studio integration is required.

## Layout example

```text
dat/stage/native-test/
  stage.json
  tiles.png
  main/
    terrain.csv
    background.csv
    foreground.csv
```

## Root

```json
{
  "formatVersion": 1,
  "id": "stage-id",
  "mode": "Action",
  "startArea": "main",
  "properties": {},
  "tileSets": [
    {
      "id": "native-test",
      "source": "tiles.png",
      "tileSize": [32, 32],
      "grid": [5, 1],
      "emptyTile": 0,
      "transparent": true,
      "terrainTiles": [
        {
          "id": 0,
          "imageIndex": 0,
          "collision": "none",
          "movement": "none"
        },
        {
          "id": 2,
          "imageIndex": 2,
          "collision": "solid",
          "movement": "none"
        }
      ]
    }
  ],
  "areas": []
}
```

### formatVersion

Required integer.

Current loader accepts only `1`.

This is the file schema version, not the game version.

### mode

Optional string.

- `Action`
- `Sokoban`

Default: `Action`.

## TileSet

```json
{
  "id": "native-test",
  "source": "tiles.png",
  "tileSize": [32, 32],
  "grid": [5, 1],
  "emptyTile": 0,
  "transparent": true
}
```

Fields:

- `id`: stable TileSet ID
- `source`: image path relative to `stage.json`
- `tileSize`: one tile's pixel size; default `[32, 32]`
- `grid`: required source-image division count `[columns, rows]`
- `emptyTile`: tile value that is not drawn; default `0`
- `transparent`: whether DxLib draws the divided image with transparency; default `true`
- `terrainTiles`: optional semantic definitions used by terrain layers

`StageData::FindTileSet()` resolves the metadata, while the actual DxLib graph handles belong to the rendering/runtime side.

### terrainTiles

Terrain CSV values are semantic tile IDs, not image indices.

```json
{
  "id": 2,
  "imageIndex": 2,
  "collision": "solid",
  "movement": "none"
}
```

Current fields:

- `id`: semantic terrain ID stored in terrain CSV
- `imageIndex`: divided-image index inside the TileSet; defaults to `id`
- `collision`: collision behavior
- `movement`: optional movement region
- `switchChannel`: optional WorldState switch channel; default `-1`
- `switchOnTileId`: semantic ID used while the channel is ON
- `switchOffTileId`: semantic ID used while the channel is OFF
- `autoTogglePeriod`: optional automatic toggle period in frames; default `0`

Supported `collision` values:

- `none`
- `solid`
- `oneWay`
- `dropThroughOneWay`
- `hitFromBelowOnly`
- `slopeUpRight`
- `slopeUpLeft`
- `stair2x1UpRightLow`
- `stair2x1UpRightHigh`
- `stair2x1UpLeftHigh`
- `stair2x1UpLeftLow`
- `stair1x2UpRightBottom`
- `stair1x2UpRightTop`
- `stair1x2UpLeftTop`
- `stair1x2UpLeftBottom`

Supported `movement` values:

- `none`
- `ladder`
- `water`
- `gravityUp`
- `gravityDown`

### WorldState binding

ON/OFF等の共有状態へ束縛するterrainは、ON/OFF両方のsemantic IDで同じbindingを持つ。

```json
{
  "id": 9,
  "imageIndex": 2,
  "collision": "solid",
  "switchChannel": 0,
  "switchOnTileId": 9,
  "switchOffTileId": 10
},
{
  "id": 10,
  "imageIndex": 0,
  "collision": "none",
  "switchChannel": 0,
  "switchOnTileId": 9,
  "switchOffTileId": 10
}
```

`switchChannel >= 0` のterrainでは `switchOnTileId` / `switchOffTileId` が必須で、同じTileSet内のsemantic IDを参照する必要がある。

`autoTogglePeriod > 0` は `switchChannel` を必要とする。NativeStageSandboxでは既存Foundation `WorldState` を使い、Stage内のArea移動でもswitch値を保持する。

The TileSet builds the existing Foundation `TileCatalog`, so CharacterController consumes native terrain through the same collision implementation already used by the Foundation sandboxes.

### rules

Native terrain definitions may contain the same `TileRule` data already consumed by Foundation's `TileBehaviorSystem`.

```json
{
  "id": 4,
  "imageIndex": 4,
  "collision": "none",
  "rules": [
    {
      "trigger": "touch",
      "action": "addCoin",
      "value": 1,
      "target": "player"
    },
    {
      "trigger": "touch",
      "action": "addScore",
      "value": 100,
      "target": "player"
    },
    {
      "trigger": "touch",
      "action": "replaceTile",
      "value": 0,
      "target": "player"
    }
  ]
}
```

Rule fields:

- `trigger`: required
- `action`: required
- `value`: optional integer, default `0`
- `once`: optional boolean, default `false`
- `countCondition`: optional, default `any`
- `countValue`: optional integer, default `0`
- `target`: optional, default `player`

Supported `trigger` values:

- `touch`
- `hitFromBelow`
- `standOn`
- `pushFromLeft`
- `pushFromRight`

Supported `action` values:

- `none`
- `replaceTile`
- `breakTile`
- `addCoin`
- `addHealth`
- `addLife`
- `addScore`
- `damage`
- `instantDeath`
- `spawnItem`
- `incrementCount`
- `toggleSwitch`
- `goal`
- `hitBrick`

Supported `countCondition` values:

- `any`
- `lessThan`
- `lessEqual`
- `equal`
- `greaterEqual`
- `greaterThan`

Supported `target` values:

- `player`
- `enemy`
- `both`

`replaceTile` and `breakTile` values must reference another semantic terrain ID defined in the same TileSet.

The loader only deserializes rules; behavior remains centralized in the existing Foundation `TileBehaviorSystem`.

## Area

```json
{
  "id": "main",
  "size": [180, 30],
  "tileSize": [32, 32],
  "settings": {},
  "tileLayers": [],
  "objectLayers": [],
  "regionLayers": [],
  "transitions": []
}
```

`size` is tile count `[width, height]`.

`tileSize` is optional and defaults to `[32, 32]`.

## Settings

```json
{
  "timeLimitSeconds": 300,
  "bgm": "stage-bgm",
  "background": "sky",
  "properties": {}
}
```

All fields are optional.

`timeLimitSeconds = 0` means no time limit unless runtime policy defines otherwise.

## TileLayer

```json
{
  "id": "terrain",
  "name": "Terrain",
  "role": "terrain",
  "tileSet": "native-test",
  "zOrder": 0,
  "visible": true,
  "source": "main/terrain.csv"
}
```

Fields:

- `id`: required stable ID
- `name`: optional; defaults to `id`
- `role`: required, `terrain` or `visual`
- `tileSet`: optional TileSet reference. Visual rendering requires it; omitting it remains valid for headless/migration StageData
- `zOrder`: optional, default 0
- `visible`: optional, default true
- `source`: required CSV path relative to stage.json

Every CSV must match the Area's `size` and `tileSize`.

The current StageData validation requires exactly one terrain layer per Area and allows zero or more visual layers.

## CSV

CSV contains only integer tile values.

```csv
0,0,0,0
0,1,1,0
1,1,1,1
```

The existing `GridDataLoader` is reused.

The committed `native-test/tiles.png` is a self-contained five-tile fixture:

- 0: transparent/empty
- 1: background
- 2: terrain block
- 3: foreground overlay
- 4: dedicated coin image

Visual-layer CSV values are direct divided-image indices.

Terrain-layer CSV values are semantic IDs. The renderer resolves them through `terrainTiles[].imageIndex`, while CharacterController resolves collision/movement through the same generated `TileCatalog`.

Do not put object/event data into TileLayer CSV.

### Number types in properties

`properties` preserves the JSON-derived stored type where possible.

- `2` -> Integer
- `2.0` -> Float when the parser receives it as a floating JSON number

However, JSON serializers may legally normalize `2.0` to `2`. Semantic float properties must therefore not depend on the textual decimal point.

`StagePropertyValue::TryGetFloat()` accepts both stored Float and Integer values, widening Integer to float. `TryGetInteger()` remains strict, and `Type()` continues to report the stored JSON-derived type.

## ObjectLayer

```json
{
  "id": "objects",
  "name": "Objects",
  "zOrder": 10,
  "objects": [
    {
      "id": "enemy-1",
      "type": "WalkingEnemy",
      "position": [320, 640],
      "properties": {
        "direction": "left",
        "speed": 2.0
      }
    }
  ]
}
```

Objects use world coordinates and are not constrained to tile boundaries.

Multiple objects may occupy the same coordinates.

`PlayerSpawn` is the native authoring convention for the player start position:

```json
{
  "id": "player-start",
  "type": "PlayerSpawn",
  "position": [32, 128]
}
```

The current NativeStageSandbox requires exactly one PlayerSpawn in the start Area. It is deliberately represented as an ObjectSpawn so the future editor can place/move it like other stage objects.

### Object runtime hitbox properties

PlayerSpawn以外のObjectSpawnはNative runtimeでHitBoundsを持てる。

任意のoverride property:

- `hitboxOffset: [x, y]`: Object positionからHitBounds左上へのoffset
- `hitboxSize: [width, height]`: 正の矩形サイズ
- `contactDamage: integer`: Player通常接触時のdamage値。0以上

例:

```json
{
  "id": "enemy-1",
  "type": "WalkingEnemy",
  "position": [224, 128],
  "properties": {
    "direction": "left",
    "speed": 2,
    "hitboxOffset": [8, 1],
    "hitboxSize": [16, 31],
    "contactDamage": 1
  }
}
```

省略時はTypeId別のruntime既定値を使う。現在の既定値:

- `WalkingEnemy`: offset `[8,1]`, size `[16,31]`, contactDamage `1`
- `HorizontalLift`: offset `[-6,11]`, size `[44,10]`, contactDamage `0`
- その他: offset `[0,0]`, size `[32,32]`, contactDamage `0`

Player側の通常接触矩形は `CharacterController::TouchBounds()` の中央16x32。矩形は半開区間として扱い、境界に触れただけではcontactにならない。

`contactDamage > 0` のObject contactはNativeStageSandboxで `DamageReactionState` を開始する。Damage reaction中の再contactはHPを減らさず、V1由来の16F reaction終了後に再びdamage可能になる。

## RegionLayer

```json
{
  "id": "events",
  "name": "Events",
  "regions": [
    {
      "id": "goal-1",
      "type": "Goal",
      "geometry": {
        "shape": "rectangle",
        "position": [5000, 400],
        "size": [64, 160]
      },
      "properties": {
        "goalKind": "normal"
      }
    }
  ]
}
```

The current native runtime recognizes `type: "Goal"`.

A Goal region requires a string property:

```json
"properties": {
  "goalKind": "normal"
}
```

Supported values are `normal` and `secret`.

The goal fires when the player's body rectangle actually overlaps the Region. Merely touching the Region boundary does not count.

Geometry:

### Point

```json
{
  "shape": "point",
  "position": [100, 200]
}
```

### Rectangle

```json
{
  "shape": "rectangle",
  "position": [100, 200],
  "size": [320, 160]
}
```

## Transition

```json
{
  "id": "pipe-1",
  "type": "Pipe",
  "entry": {
    "shape": "point",
    "position": [144, 160]
  },
  "targetArea": "sub",
  "exitPosition": [320, 160],
  "enterDirection": "down",
  "exitDirection": "up"
}
```

Optional `targetStage` selects another StageData.

When `targetStage` is omitted or empty, `targetArea` must exist in the current StageData.

For `type: "Pipe"`, the NativeStageSandbox bridges `enterDirection / exitDirection` to the existing Foundation `PipeTransport`. For an in-stage cross-Area transition, the active Area is switched at full fade-out and the player then emerges at `exitPosition` in the target Area.

External `targetStage` transitions remain valid data, but loading another StageData is not yet connected to the Sandbox runtime.

Directions:

- `none`
- `up`
- `down`
- `left`
- `right`

## Typed properties

`properties` may be added to Stage, Area settings, Object, Region, and Transition.

JSON value mapping:

| JSON | StagePropertyValue |
|---|---|
| integer | Integer |
| decimal number | Float |
| boolean | Boolean |
| string | String |
| two-number array | Vector2 |

Example:

```json
{
  "direction": "left",
  "variant": 1,
  "speed": 2.0,
  "enabled": true,
  "pathDelta": [192.0, 0.0]
}
```

Nested objects and arbitrary arrays are intentionally rejected.

Complex authoring data should receive a real schema/type rather than becoming an untyped JSON tree.

## Loader

```cpp
Result<StageData> Loaded =
    NativeStageDataLoader::Load("dat/stage/5/stage.json");
```

Load sequence:

```text
stage.json
   ↓ nlohmann/json
tile sets / metadata / objects / regions / transitions
   ↓
TileLayer.source
   ↓ GridDataLoader
CSV
   ↓ TileMap::Create
StageData
   ↓ ValidateStageData
validated native stage
```

## Editor direction

The editor should operate on `StageData`, not directly on JSON DOM nodes.

JSON/CSV is serialization.

This keeps:

- editor model
- game runtime model
- migration converter output

on the same native StageData representation.

A later serializer can write StageData back to JSON/CSV without changing runtime structures.
