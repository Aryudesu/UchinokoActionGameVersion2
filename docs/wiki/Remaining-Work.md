# 残件・未移植・設計判断

2026-09-21時点。

---

## 1. 進行中

### PR #28 — Version1 ARY互換ローダー

Version1の `map*.ary / img*.ary` をFoundation `LayeredMap` へ取り込む互換層。

目的:

- 既存Version1ステージ資産を失わない
- Player spawn / Enemy spawnを抽出
- Terrain / Visualを分離
- HSP由来の無効コードを消さずmetadataへ保持

PR #28は現時点で未マージ。

---

## 2. 次の優先残件

推奨順:

1. PR #28の安定化・マージ
2. Version1 `Data{detail}.inf` loader
3. V1 Block ID 0..45 → Version2 TileCatalog mapping
4. `Action::LoadMapData` からFoundation経路を作る
5. Foundation Effect → Player/GameData/SE adapter
6. PlayerをCharacterControllerへ段階移行
7. ObjectSpawn型を新設
8. Legacy Enemy `-2..-6` → ObjectSpawn
9. Pipe / stage transitionを本編へ接続
10. Goal / StageProgress → GameData / Save / WorldMap
11. Lift / moving platform
12. Enemy runtime
13. Boss / Event
14. 旧Map / Block runtime撤去

---

## 3. Foundation実装済みだが本編未統合

- CharacterController
- TileDefinition / TileInteraction
- TileRuntimeMap
- ConditionalTerrain
- BrickSystem
- ItemSystem
- PipeTransport
- GoalState
- StageProgress
- LayeredMap
- WorldState

この「実装済み」と「ゲームで使用中」を混同しないこと。

---

## 4. 未移植

### Version1現役機能

- Enemy / ObjectManager
- Lift / moving object
- Boss
- presentation effect
- Score popup
- Fragment描画
- SE/BGM接続の新runtime化
- Save
- WorldMap
- Scene/Event

### HSPに存在した旧機能

要否判断が必要:

- Star Coin
- HSP checkpoint
- 321..324 sequence 1UP
- HSP固有Enemy
- HSP固有Gimmick
- shooting
- bomb
- STG mode
- HSPワールドマップの細かな進行状態

---

## 5. 明確に復活させない旧構造

現時点の設計判断。

### HSPの1セル1値

復活しない。

Terrain / Visual / Object / Eventを分ける。

### HSPの mov + coo + inf runtime

復活しない。

必要なら旧データimport時のみ解釈し、Version2の直接Transitionへ変換。

### Version1の Block* 2次元配列

最終的には撤去予定。

### HSP/V1コード番号をVersion2正式IDに固定

しない。

---

## 6. ステージデータの未決事項

- Object配置をCSV / INI / JSONのどれにするか
- EventをObjectsと同じファイルへまとめるか
- LiftのpathをObject内paramで持つか別定義にするか
- Visualを必須グリッドにするか、override扱いにするか
- 背景装飾レイヤをVisualと別にするか
- submap移動先を整数IDにするかDefinition IDにするか
- Legacy loaderを最終製品にも残すか、offline converterへ移すか

---

## 7. 現時点の推奨設計判断

### Terrain

2次元グリッド。

### Visual

2次元グリッドまたはTerrainのImageIndexに対するoverride。

### Enemy / Lift / Dynamic Object

座標付き配置リスト。

### Event / Trigger

座標・領域付き配置リスト。

### Pipe / Transition

入口→移動先→出口を明示。

### Legacy

V1形式はcompatibility/import層。

新規V2ステージをV1制約へ合わせない。

---

## 8. 調査資料の扱い

旧HSPファイルから新しい事実が分かった場合、

1. HSP仕様ページへ「旧仕様」として記録
2. Version1でどう変化したか確認
3. Version2へ必要か判断
4. 必要ならVersion2設計として別途記録

という順にする。

**旧コードに存在することと、Version2で実装すべきことは別。**
