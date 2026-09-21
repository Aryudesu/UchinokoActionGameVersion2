# 設計判断ログ

将来「なぜこの形にしたのか」を追えるように、重要な判断と理由を残します。

## ADR-001: Terrain collisionの正本をCanvasMasao系へ寄せる

**状態:** 採用

### 理由

HSP/V1には世代ごとの直接座標補正が多く、坂処理の再現が保守しづらい。

Version2ではCanvasMasao由来の地形仕様を明示的なCollisionShapeとして再構築する。

### 影響

HSP/V1コードは旧操作感の参考資料には使うが、見つけた処理をそのままCharacterControllerへ追加しない。

---

## ADR-002: V1資産の変換規則ではC++ V1 runtimeを正とする

**状態:** 採用

### 理由

正規V1データ内にはHSPコードが残存しているが、V1 runtimeでは無効。

HSP由来の意味を復活させると、V1資産をV2へ変換した結果がV1での実際の挙動と変わってしまう。

ただし、これはVersion2 runtimeにV1互換性を持たせるという意味ではない。

### 例

`110 / 301 / 321..325 / -79` はmetadataとして保存してもTerrainでは有効化しない。

---

## ADR-003: HSPのmov + coo + infをVersion2 runtimeへ復活させない

**状態:** 採用

### 理由

入口、移動先、座標、方向、ステージ設定が複数ファイルへ分散し複雑。

Version1でも `StageMoving + AppearData + Data*.inf` へ一段簡単化されている。

Version2では入口→出口/Targetを明示するTransitionを採用する。

---

## ADR-004: Terrain / Visualを分離する

**状態:** 採用

### 理由

Collision/behaviorと見た目は独立した概念。

Version1でも `map*.ary / img*.ary` として既に分離されている。

### 影響

Legacy importでは両方を保持する。

V2 nativeではVisualをfull gridにするかoverrideにするかは未決。

---

## ADR-005: Enemy / LiftをTerrain gridへ押し込まない

**状態:** 方針採用、型は未実装

### 理由

2次元gridの1セル1値では、

- TerrainとEnemyの重なり
- EnemyとLiftの重なり
- 複数Object
- sub-tile座標
- path/speed/direction parameter

が扱いづらい。

### 方針

V2 nativeは `ObjectSpawn[]` のような座標付き配置リストを使う。

`LayeredMap.Object` はLegacy中間表現として残してよい。

---

## ADR-006: Tile gimmickを継承階層ではなくRule/Effectへ分解する

**状態:** 採用・Foundation実装済み

### 理由

Version1ではCoinBlock/HealingBlock/OneUpBlock等の派生クラスが増殖していた。

Version2では、

```text
TileDefinition
  + TileRule
      ↓
TileInteraction
      ↓
TileEffect
```

で合成可能にする。

FoundationはManagerへ直接依存しない。

---

## ADR-007: Normal / Secret clearを別状態として保存する

**状態:** 採用・Foundation実装済み

### 理由

HSPには通常/裏ゴールの区別があったが、Version1のBeatLevelでは最終的に区別が失われた。

Version2では `GoalKind` と `StageClearState` で明示する。

---

## ADR-008: Legacy parser/converterとV2 native runtimeを分ける

**状態:** 採用

### Legacy

既存HSP/V1資産を解析してV2形式へ変換するための入力側。

### Native

Version2 runtimeが通常利用する整理済みデータ。

### 理由

新規ステージまでV1の制約へ合わせると、移植後も古い構造を永久に背負うため。

---

## ADR-009: 旧コード番号をV2正式IDとして固定しない

**状態:** 採用

### 理由

HSP、V1、Foundation test dataでID体系が異なる。

番号自体より意味をmappingする。

---

## ADR-010: Foundation実装と本編統合を別ステータスで管理する

**状態:** 採用

### 理由

現在、FoundationにはCharacterControllerやPipeTransport等が存在するが、Action/PlayerManager等は旧runtime。

「実装済み」と書くだけでは誤解するため、

- Foundation実装済み
- Runtime統合済み
- Legacy importerのみ
- 未実装

を区別する。


---

## ADR-011: Version2 runtimeは旧形式との後方互換を持たない

**状態:** 採用

### 方針

HSP/V1のデータを将来利用する必要がある場合は、

```text
Legacy data
   ↓
parser / converter
   ↓
V2 native data
```

へ変換する。

Version2本体へ、

- V1 block ID分岐
- HSP map code分岐
- `StageMoving`
- `AppearData`
- `mov/coo/inf`

等の旧形式都合を恒久的に持ち込まない。

### 理由

後方互換が製品要件ではないため、runtimeを単純に保つ利益の方が大きい。

旧ステージを残したい場合も一度変換できればよく、旧形式を毎回実行時に解釈する必要はない。

### PR #28の位置付け

`LegacyStageLoader` のコードは無駄ではない。

旧ARYの意味を解析し、

- Terrain
- Visual
- Player spawn
- Enemy spawn
- 未解釈marker

へ分解できるため、**runtime互換ローダーではなく変換器の入力parser / 中間表現生成器**として利用できる。
