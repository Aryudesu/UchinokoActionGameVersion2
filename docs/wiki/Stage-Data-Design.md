# Version2 ステージデータ設計案

> このページはVersion2 native stage dataの設計記録です。
>
> PR #30で、保存形式に依存しないメモリ上の基本モデルをFoundationへ実装しました。
> `LayeredMap` / `TerrainStageLoader` は既存の別系統として残っています。

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

## 2. 採用した責務分離

PR #30では次の型を追加。

- `StageData`
- `StageArea`
- `TileLayer`
- `ObjectLayer / ObjectSpawn`
- `RegionLayer / StageRegion`
- `StageTransition`
- `StagePropertyValue`



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

### PR #30時点

V2 nativeの `TileLayer` は複数枚持てる。

- `Terrain` role: Areaごとに正本1枚
- `Visual` role: 0枚以上

Background / Decoration / Foreground等はVisual layerを複数枚作り、`ZOrder` で重ねる。

「標準画像 + sparse override」への保存最適化はserializer/editor側で後から検討でき、メモリモデル自体は複数TileLayerを正規表現とする。

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

### 実装済みの形

```cpp
struct ObjectSpawn {
    std::string Id;
    std::string TypeId;
    WorldPosition Position;
    StagePropertyMap Properties;
};
```

Object/Region/Transitionはstable IDを持ち、Area内で一意にする。

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

これらもタイル1セルである必要はない。

PR #30では `StageRegionGeometry` として、

- Point
- Rectangle

を実装。

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

### PR #30実装

```cpp
struct StageTransition {
    std::string Id;
    std::string TypeId;
    StageRegionGeometry Entry;

    std::string TargetStageId; // empty = same stage
    std::string TargetAreaId;

    WorldPosition ExitPosition;
    StageDirection EnterDirection;
    StageDirection ExitDirection;
    StagePropertyMap Properties;
};
```

同一Area、別Area、別Stageを同じモデルで表現できる。

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

はLegacy V1データを4種類に分類する**変換時の中間表現**として非常に有用。

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

旧資産parser / converterは既存資産を必要に応じて救済するために利用する。

**Version2 runtimeがLegacy形式を直接サポートする必要はない。**

変換後は旧ステージも新規ステージもV2 native形式として扱う。

---

## 12. 現時点の判断

### PR #30で採用・実装

- TerrainとVisualを分離
- ObjectはTerrainから分離
- EnemyとLiftは同じ「Object系」でもよい
- Objectはグリッドではなく配置リスト
- Eventも独立
- Pipe/Transitionは直接リンク
- Legacy形式は変換入力。runtime互換対象にはしない

### PR #31で採用

- native serializer v1: JSON + CSV
- JSON: Stage / Area / Layer定義 / Object / Region / Transition / Settings
- CSV: TileLayerの整数grid
- JSON parser: nlohmann/json v3.12.0 single-headerを `third_party/` にvendor
- `formatVersion = 1` を必須化

### PR #32で追加

- `TileSetDefinition`: image / tileSize / grid / emptyTile / transparency
- TileLayer → TileSet ID参照
- Tile/Object/RegionをZOrder横断で描画するNativeStageSandbox
- Object / Region / Transitionのdebug visualization
- 画像handleはStageDataへ持たせずDxLib側へ隔離

現在のSandboxではCSVの非empty値をTileSetの画像indexとして直接描画する。
Terrainの意味ID → `TileDefinition.ImageIndex` はnative gameplay接続前に別途設計する。

### 未決

- 将来binary/export formatを追加するか
- TileLayerを保存時にfull gridにするか圧縮するか
- TypeIdごとのproperty schema
- Lift path表現
- editor project / undo-redo / clipboard等のeditor固有状態
- runtimeがStageDataを読み込むbridge


---

## 13. 将来拡張: スクロール / パララックスTileLayer

すべてのステージで必要ではないが、将来的に一部ステージで、

- 遠景
- 雲
- 前景
- 装飾

等を通常のWorld scrollとは異なる速度で動かしたい可能性がある。

この場合、`StageData` に専用の「スクロールレイヤー」を1個追加するのではなく、**任意のTileLayerへoptionalなスクロール特性を持たせる**方針を採る。

概念例:

```cpp
struct LayerTransform {
    WorldPosition Offset;
    WorldPosition ScrollFactor { 1.0f, 1.0f };
};
```

例:

```text
ScrollFactor = (1.0, 1.0)  通常World layer
ScrollFactor = (0.5, 1.0)  横方向パララックス
ScrollFactor = (0.0, 0.0)  Camera固定
ScrollFactor = (1.2, 1.0)  手前側
```

PR #30ではまだ実装しない。

### 地形そのものが動く場合

Visual layerのパララックスと、Collisionを持つTerrain layer自体の移動は別問題。

PR #30では現在、

> AreaごとにTerrain TileLayerはちょうど1枚

をvalidation条件としている。

将来、**TileMap全体が独立移動し、そのTileMapへ当たり判定も付く**ステージを作る場合は、この制約を再検討する。

ただし、

- Lift
- MovingPlatform
- 小規模な動く足場

は `ObjectSpawn` として表現した方が自然。

そのため、

```text
見た目だけ独立スクロール
    → TileLayer propertyで対応予定

小規模な動く地形
    → Object / MovingPlatformを優先

大規模なTileMapそのものが動く
    → Moving TileLayer等を将来検討
```

とする。

この機能はoptionalであり、不要なStageへ追加設定を強制しない。


---

## 14. Native file format v1

PR #31で最初のnative file formatを実装。

```text
stage.json
  ├ Stage / Area / Settings
  ├ TileLayer metadata
  ├ ObjectLayer / ObjectSpawn
  ├ RegionLayer / StageRegion
  └ Transition

*.csv
  └ TileLayer grid
```

格子状データだけCSV、構造化データはJSONへ置く。

`TileLayer.source` は `stage.json` からの相対パス。

Loader:

```cpp
NativeStageDataLoader::Load("dat/stage/.../stage.json")
```

で、

```text
JSON
 + CSV
   ↓
StageData
   ↓
ValidateStageData
```

まで行う。

EditorはJSON DOMを直接編集するのではなく、同じ `StageData` を編集対象とする。
