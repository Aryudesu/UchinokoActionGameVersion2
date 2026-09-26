# 残件・未移植・設計判断

2026-09-26時点。

---

## 1. 進行中

### PR #28 — Version1 ARY移行parser

Version1の `map*.ary / img*.ary` を解析してFoundation `LayeredMap` へ分解する移行用parser。

最終runtimeで旧ARYを直接読み続けるための互換層にはしない。

目的:

- 既存Version1ステージ資産を失わない
- Player spawn / Enemy spawnを抽出
- Terrain / Visualを分離
- HSP由来の無効コードを消さずmetadataへ保持

PR #28は現時点で未マージ。

---

## 2. 次の優先残件

PR #40〜#46までで、Native Sandbox上のWalkingEnemy / Camera / lifecycle / CarrotManまで実装済み。

推奨順:

1. Version1 `BallSlime / BallSlime2` をBehaviorStateベースで移植
2. Enemy死亡演出 / Score / SEをruntime effectへ分離
3. HSP由来の飛行Enemy・ジャンプEnemyをNative Enemy型として整理
4. Projectile基盤を作り、固定砲台 / ピカチー等の射撃Enemyへ展開
5. Lift Stand判定 / moving platform runtimeを完成
6. external Stage transition / Stage loader責務を設計
7. PR #28を旧ARY parser / converter入力として整理
8. Version1 `Data{detail}.inf` parserを変換ツール側へ追加
9. V1 Block ID 0..45 → V2 native Tile定義への変換mapping
10. 旧Stage 5をV2 native形式へ実際に変換するfixtureを作る
11. `Action` をV2 native StageDataから動かす
12. Legacy Enemy `-2..-6` → ObjectSpawn変換
13. Goal / StageProgress → GameData / Save / WorldMap
14. Boss / Event
15. 旧Map / Block runtime撤去

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

- Enemy: WalkingEnemy1/2とCarrotManはNative化済み。BallSlime系・HSP特殊Enemyは未移植
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

PR #30〜#46はdevへマージ済み。WalkingEnemy1/2、踏みつけ、Enemy同士衝突、Enemy対象Damage terrain、Camera lifecycle、CarrotManまでNativeObjectRuntimeへ接続済み。

- JSON + CSVをauthoring/native v1として採用済み。将来binary/export formatを追加するか
- TileLayer CSVを将来full grid / sparse / chunkedへ最適化するか
- TypeIdごとのproperty schema
- Lift pathをObject propertyで持つかPath/Region等を別概念にするか
- editor固有のvisibility / lock / selection等をstage dataと分離するか
- Legacy parserは原則offline converter / 開発ツール側へ置き、製品runtimeには残さない

---

## 7. 現時点の推奨設計判断

### Terrain

2次元グリッド。

### Visual

PR #30ではVisual TileLayerを複数枚持ち、ZOrderで重ねる。

保存形式側でfull grid / sparse等へ最適化する余地は残す。

### Enemy / Lift / Dynamic Object

座標付き配置リスト。

Playerとの通常接触は `CharacterController::TouchBounds()` の中央16x32を基本とし、Object側はTypeIdごとの固有HitBoundsを持てる設計にする。Liftの乗り判定は通常接触とは分離して足元/Stand判定を使う。

### Event / Trigger

座標・領域付き配置リスト。

### Pipe / Transition

入口→移動先→出口を明示。

### Legacy

V1形式は**移行入力**。

旧形式を直接runtimeで使い続ける後方互換は不要。

新規V2ステージも、変換済み旧ステージも最終的には同じV2 native形式を使う。

---

## 8. 調査資料の扱い

旧HSPファイルから新しい事実が分かった場合、

1. HSP仕様ページへ「旧仕様」として記録
2. Version1でどう変化したか確認
3. Version2へ必要か判断
4. 必要ならVersion2設計として別途記録

という順にする。

**旧コードに存在することと、Version2で実装すべきことは別。**
