# Version2 ステージデータ設計案

> このページは **提案中の設計** です。
>
> 現在の `LayeredMap` / `TerrainStageLoader` の実装仕様そのものではありません。

---

## 1. 現在の問題

HSP版・Version1のmapは、歴史的に「1セルに1つの主値」を置く構造です。

そのため、

- Solid blockの上にEnemyを置く
- 地形とLiftを同じ位置へ置く
- 同じ座標に複数Objectを置く
- タイル中央からずらしてEnemyを置く

等が自然に表現しにくい問題があります。

Version1では `img*.ary` を分けることで見た目は独立しましたが、Enemy spawnはまだ `map*.ary` の `-2..-6` に混在しています。

---

## 2. 推奨する責務分離

```text
StageData
 ├─ Terrain
 ├─ Visual
 ├─ Objects[]
 ├─ Events[]
 ├─ Transitions[]
 └─ Settings
```

---

## 3. Terrain

### 形式

2次元TileMap。

### 責務

- Solid
- OneWay
- Slope
- Ladder
- Water
- GravityRegion
- Damage/InstantDeath
- Conditional terrain
- ON/OFF等のTile behavior

例:

```text
terrain0.ary
```

タイル単位であること自体に意味があるため、グリッドが自然。

---

## 4. Visual

### 形式案A: 完全グリッド

Version1と同様、

```text
visual0.ary
```

をTerrainと同サイズで持つ。

長所:

- 既存V1 img*.aryをそのまま変換しやすい
- 同じTerrain IDでもセルごとに画像を変えられる
- editorが単純

### 形式案B: override

通常は `TileDefinition.ImageIndex` を使用し、特殊セルだけVisual override。

長所:

- データ量が少ない
- 「Tile ID→標準画像」が自然

### 現時点の推奨

Legacy importでは完全Visual gridを保持。

V2ネイティブデータでは、editor都合を見ながらどちらか決める。

---

## 5. Objects

Enemy、Lift、moving platform、動的Gimmick等は **TileMapにしないことを推奨**。

### 理由

- Terrainと重ねられる
- 同じ位置へ複数置ける
- 32px境界に縛られない
- Objectごとにparameterを持てる
- 将来的な大きさ・path・速度・方向へ拡張できる

### 例

```csv
type,id,x,y,param1,param2
Enemy,Walking,320,640,Left,
Enemy,Flying,480,400,,
Lift,Horizontal,800,600,range=192,speed=2
```

実際のformatは未決定。

### 将来の型イメージ

```cpp
struct ObjectSpawn {
    ObjectKind Kind;
    std::string TypeId;
    WorldPosition Position;
    ObjectParameters Parameters;
};
```

---

## 6. Events

候補:

- Normal Goal
- Secret Goal
- Checkpoint
- Camera trigger
- BGM trigger
- Scene event
- Spawn trigger
- Stage transition

これらもタイル1セルである必要はありません。

Point / Rectangle等のTrigger領域を持てる構造が望ましい。

---

## 7. Transitions / Pipe

HSP:

```text
mov + coo + inf
```

Version1:

```text
StageMoving + AppearData + Data*.inf
```

Version2:

```text
Entry
  ↓
Target Stage/Submap
  ↓
Exit
```

を直接定義。

### 将来案

```cpp
struct StageTransition {
    WorldPosition EntryPosition;
    PipeDirection EnterDirection;

    StageAreaId Target;
    WorldPosition ExitPosition;
    PipeDirection ExitDirection;
};
```

Targetが現在areaと同じなら現在の `PipeLink` と同じ。

---

## 8. Settings

ステージ/areaごとに、

- BGM
- Background
- Time
- Scroll mode / camera
- Tile size
- Asset references

等を保持。

HSPの `inf`、Version1の `Data*.inf` に相当します。

---

## 9. ファイル構成例

```text
dat/stage/5/
  stage0.ini
  terrain0.ary
  visual0.ary
  objects0.csv
  events0.csv
  transitions0.csv

  stage1.ini
  terrain1.ary
  visual1.ary
  objects1.csv
  events1.csv
  transitions1.csv
```

ただし、**概念を分けることとファイルを必ず分けることは別**です。

例えばJSONを採用するなら、

```text
stage0.json
terrain0.ary
visual0.ary
```

として、objects/events/transitionsをstage0.jsonへまとめる設計も可能。

---

## 10. LayeredMapとの関係

現在の `LayeredMap`:

- Terrain TileMap
- Visual TileMap
- Object TileMap
- Event TileMap

はLegacy V1データを4種類に分類する中間表現として非常に有用。

一方、V2ネイティブ形式では、

```text
LegacyStageLoader
      ↓
LayeredMap
      ↓
Object/Event抽出
      ↓
StageData
```

とし、最終runtimeではObject/Eventをリストへ変換する案が扱いやすい。

---

## 11. Legacy import

最終的な方向:

```text
Version1
 Data*.inf
 map*.ary
 img*.ary
       ↓
 Legacy importer
       ↓
Version2
 Terrain
 Visual
 ObjectSpawn[]
 Event[]
 Transition[]
 Settings
```

互換ローダーは既存資産救済のために必要。

ただし新規ステージまでLegacy形式で作り続ける必要はありません。

---

## 12. 現時点の判断

### 採用方向

- TerrainとVisualを分離
- ObjectはTerrainから分離
- EnemyとLiftは同じ「Object系」でもよい
- Objectはグリッドではなく配置リスト
- Eventも独立
- Pipe/Transitionは直接リンク
- Legacy形式はimport対象

### 未決

- Object/Eventの実ファイル形式
- Visual override方式
- StageAreaIdの型
- Lift path表現
- editorとの連携方式
