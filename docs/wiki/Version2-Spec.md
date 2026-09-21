# Version2仕様

2026-09-21時点の `dev` と、作業中PRを分けて記載します。

Version2は **Version1のクラス構造をそのままコピーするのではなく、Foundation層へ意味ごとに分解して再構成する** 方針です。

---

## 1. 現在の大枠

```text
旧runtime
Action
 ├─ Map
 ├─ PlayerManager
 ├─ ObjectManager
 └─ GameData

          ↓ 段階移行中

Foundation
 ├─ TileMap / TileCatalog
 ├─ TerrainCollision
 ├─ CharacterController
 ├─ TileInteraction
 ├─ ConditionalTerrain
 ├─ BrickSystem
 ├─ ItemSystem
 ├─ PipeTransport
 ├─ GoalState
 ├─ StageProgress
 ├─ LayeredMap
 └─ WorldState
```

Foundationの機能が実装されていても、**本編 `Action.cpp` からまだ利用されていないものがあります**。

---

## 2. TileMap / TileDefinition

### TileMap

タイルIDの2次元グリッド。

タイルサイズを保持し、world座標とtile座標を変換します。

### CollisionShape

現在の主な形状:

- None
- Solid
- OneWay
- DropThroughOneWay
- HitFromBelowOnly
- SlopeUpRight
- SlopeUpLeft
- 2x1 stair 4種
- 1x2 stair 4種

坂・階段形状はCanvasMasao由来の地形仕様を基準に実装しています。

### MovementRegion

衝突形状とは別に、

- Ladder
- Water
- GravityUp
- GravityDown

を保持できます。

### TileDefinition

1タイルの定義には、

- ID
- ImageIndex
- CollisionShape
- Rules
- Switch binding
- Auto toggle period
- GameState条件
- MovementRegion

を持ちます。

---

## 3. TileInteraction

Version1のBlock派生クラスに埋め込まれていた、

- `Touched`
- `Hited`
- `On`
- `PushL`
- `PushR`

等を、Version2ではデータ駆動のルールへ分解します。

### Trigger

- Touch
- HitFromBelow
- StandOn
- PushFromLeft
- PushFromRight

### Action

- ReplaceTile
- BreakTile
- AddCoin
- AddHealth
- AddLife
- AddScore
- Damage
- InstantDeath
- SpawnItem
- IncrementCount
- ToggleSwitch
- Goal
- HitBrick

### Foundationの責務

Foundationは `SoundManager`, `GameData`, `PlayerManager` を直接呼びません。

代わりに `TileEffect` を返し、外側のruntime adapterが、

- PlayerへHPを足す
- Coinを足す
- SEを鳴らす
- Scoreを足す
- Goal処理を行う

等へ接続する設計です。

---

## 4. CharacterController

Player物理・地形接触を旧 `Player.cpp + Map*` から独立させるためのFoundation実装。

現在扱う主な要素:

- 通常横移動
- Jump
- Gravity
- Grounded
- Wall collision
- CanvasMasao準拠のSlope追従
- Ladder
- Ladder jump
- Water
- Gravity Up / Down
- OneWay
- DropThroughOneWay
- TileInteraction生成

水中用パラメータも `CharacterMotion` に分離されています。

---

## 5. LayeredMap

現在の `dev` には次の4レイヤがあります。

```cpp
enum class MapLayerKind {
    Terrain,
    Visual,
    Object,
    Event
};
```

現時点では4つとも `TileMap` として保持します。

### 用途

- Terrain: 衝突・地形
- Visual: 見た目
- Object: 旧データのObject配置情報
- Event: 旧データのEvent/未解釈値等

ただし **Version2ネイティブの最終Object形式まで確定したわけではありません**。

Enemy / Lift等については、グリッドではなく座標付き配置リストへ移す案を推奨しています。

詳細は [Version2 ステージデータ設計案](Stage-Data-Design.md)。

---

## 6. TerrainStageLoader

現在のネイティブFoundationテスト用ステージ形式。

`TerrainStageData`:

```cpp
struct TerrainStageData {
    TileMap Map;
    TileCatalog Catalog;
    std::vector<PipeLink> Pipes;
    WorldPosition PlayerSpawn;
};
```

manifestから、

- terrain
- tiles
- spawn_x / spawn_y
- tile_width / tile_height
- pipes

等をロードできます。

Tile定義はCSVで外出しされています。

これはFoundation検証用として機能していますが、**最終的な本編ステージフォーマットは今後LayeredMap/Object配置等を含めて再整理予定**です。

---

## 7. PipeTransport

Version2ではHSP/V1の位置依存・サブマップ配列方式を簡単化しています。

現在の `PipeLink`:

```cpp
struct PipeLink {
    WorldPosition EntryPosition;
    PipeDirection EnterDirection;
    WorldPosition ExitPosition;
    PipeDirection ExitDirection;
};
```

Phase:

- Idle
- Entering
- FadeOut
- FadeIn
- Emerging

入口と出口を直接結びます。

### 現時点の制限

現在の `PipeLink` 自体にはTargetMapIdがないため、同一読み込み済みステージ内の直接移動モデルです。

別サブマップ対応を本編へ入れる場合も、旧 `mov/coo` 方式へ戻さず、

- TargetMapId / StageDetailId
- ExitPosition
- ExitDirection

程度を明示的に持つ方式を想定します。

---

## 8. Block / Gimmick系 Foundation実装

現在までにFoundationへ移植・再実装済みの主なもの:

- ? block
- pop-out item
- 10 coin block
- ON/OFF
- ON/OFF連動地形
- crushing判定
- timed appearing/disappearing block
- coin-count conditional terrain
- ladder
- ladder maker
- water
- gravity reverse
- ladder jump
- one-way / drop-through
- BrickSystem
- damage block
- instant-death block
- Coin
- HealingCoin
- OneUPCoin

### BrickSystem

Version1互換値を明示保持:

- 必要HP: 5
- bump: 9 frames
- break: 7 frames
- break score: 10
- fragments: 5

### ItemSystem

Foundation上のItemKind:

- Coin
- Healing
- OneUp
- LadderBuilder

Tileの `SpawnItem` Effectから生成します。

---

## 9. Goal / StageProgress

Version1では `BeatLevel` に「クリア済み」を1種類だけ保存していました。

Version2では通常/裏ゴールを分離しています。

### GoalKind

- Normal
- Secret

### StageCompletionState

1プレイ中のクリア結果。

### StageClearState

永続履歴:

- NormalCleared
- SecretCleared

### ClearRequirement

- Normal
- Secret
- Either
- Both

`StageProgress` がステージ単位の履歴を保持します。

**まだ旧GameData/Save/WorldMapへの本編統合は残っています。**

---

## 10. 旧ARY互換 — PR #28 作業中

PR #28ではVersion1の、

- `map*.ary`
- `img*.ary`

を `LayeredMap` へ取り込む `LegacyStageLoader` を追加中です。

予定される分離:

```text
0..45      → Terrain
img*.ary   → Visual
-1         → PlayerSpawn
-2..-6     → Enemy spawn情報
その他     → Terrain上はEmpty
             元値をEvent/Unresolved metadataへ保持
```

重要:

HSP時代に意味があった `100 / 110 / 301 / 321..325 / -79` 等も、**C++ Version1互換としてはEmpty** とします。

HSP機能を勝手に復活させず、元コードだけ調査用に保持します。

---

## 11. まだ旧runtime側に残っているもの

主なもの:

- `Action::LoadMapData`
- `Map`
- `Block*` runtime
- `PlayerManager / Player`
- `ObjectManager`
- Enemy runtime
- Lift / moving object
- Boss
- presentation effects
- SE/BGM接続
- GameData save
- WorldMap

つまりVersion2は現在、

> Foundationの土台を先に作り、その後本編runtimeを段階的にFoundationへ接続する

フェーズです。

---

## 12. Version2で意図的に変えた点

### Terrain collision

旧HSP/V1の座標補正をそのまま再現せず、CanvasMasao系仕様へ再構築。

### Pipe

`mov + coo + inf` や `StageMoving + AppearData` を正式仕様にせず、直接リンク方式。

### Goal

通常/Secretを分離。

### Tile gimmick

Block派生クラス増殖ではなく、TileDefinition + Rule + Effect。

### 旧コードID

HSP/V1の番号をVersion2正式IDとして固定しない。

---

## 13. Version2の現在の設計目標

```text
Stage
 ├─ Terrain
 ├─ Visual
 ├─ ObjectSpawn[]
 ├─ Event[]
 ├─ Transition[]
 └─ Settings
```

ただしObjectSpawn/Eventのネイティブ形式はまだ未実装・未確定です。

詳細は [Version2 ステージデータ設計案](Stage-Data-Design.md)。
