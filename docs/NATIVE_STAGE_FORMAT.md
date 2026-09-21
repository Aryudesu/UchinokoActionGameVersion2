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
  "zOrder": 0,
  "visible": true,
  "source": "main/terrain.csv"
}
```

Fields:

- `id`: required stable ID
- `name`: optional; defaults to `id`
- `role`: required, `terrain` or `visual`
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

Do not put object/event data into TileLayer CSV.

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
metadata / objects / regions / transitions
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
