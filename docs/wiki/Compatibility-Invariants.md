# 旧資産変換時の不変条件

HSP / Version1資産をVersion2形式へ**変換・解析するとき**に、旧データの意味を取り違えないための条件です。

これはVersion2 runtimeの後方互換要件ではありません。
V2本体が `map*.ary`, `img*.ary`, `Data*.inf` を恒久的に直接読める必要はありません。

## 1. 「HSP互換」と「C++ Version1互換」を混同しない

V1資産を変換するときの入力仕様として正とするのは **C++ Version1**。

HSPコードの意味が判明していても、C++ Version1で無効だった値を勝手に有効化しない。

例:

- `110`
- `301`
- `321..325`
- `-79`

HSPでは意味があるが、C++ V1 `Map::LoadMap` ではEmpty。

必要なら元コードをmetadataとして保存する。

## 2. C++ V1 Map::LoadMapの解釈

- `0..50` → BlockFactory
- `-1` → Player spawn
- `-2..-6` → EnemyKind 1..5
- その他 → Empty

ただしBlockFactoryで意味を持つのは `0..45`。

したがって `46..50` も実質Empty。

## 3. Player spawn

Version1はmap走査中に `SetInitPos` を呼ぶ。

複数の `-1` が存在した場合、後から読んだものが上書きするため、**row-major走査で最後のspawnが有効**。

変換器はこのルールで入力を解釈し、最終的には単一のV2 PlayerSpawnへ正規化する。

## 4. Enemy marker

- `-2` → EnemyKind 1
- `-3` → EnemyKind 2
- `-4` → EnemyKind 3
- `-5` → EnemyKind 4
- `-6` → EnemyKind 5

TerrainとしてはEmptyにする。

将来的には `ObjectSpawn[]` へ変換する。

## 5. Visualは挙動から独立

Version1 `img*.ary` は `map*.ary` から生成されたBlockの見た目を上書きする。

変換時にはTerrainとVisualを別概念として取り出し、V2ネイティブ表現へ正規化する。

## 6. HSPコードは消さない

HSP由来でV1では無効な値も、調査・将来復元のため元値を保持する。

ただし保持場所はTerrainではなく、Event/Unresolved metadata等。

## 7. V1 block IDをV2正式IDとみなさない

Version1 ID 0..45はLegacy mapping用。

V2のTileCatalog ID体系とは別概念として扱う。

## 8. Pipe/submap移動

Legacyデータを読むために、

- `StageMoving`
- `AppearData`

を解釈することはある。

しかしVersion2 native runtimeへHSPの `mov/coo/inf` やV1のX位置依存ルールを持ち込まない。

最終的には明示的Transitionへ変換する。

## 9. CanvasMasao terrain semantics

坂・基本地形collisionのVersion2正本はCanvasMasao由来Foundation。

HSP/V1の古い座標補正を発見しても、自動的にCharacterControllerへ戻さない。

## 10. Foundationの依存方向

Foundationは、

- GameData
- PlayerManager
- SoundManager
- ImageManager

を直接呼ばない。

意味のあるEffect/Stateを返し、外側のadapterがゲームruntimeへ接続する。

## 11. Legacy parser/converterとNative runtimeを分ける

### Legacy parser / converter

旧資産を正しく**読むための一時的な入口**。

### Native V2 runtime

新設計だけを使う。

理想形では、変換済みステージを本番runtimeが読み、Legacy parserは開発ツール・offline converter側に隔離する。

旧形式の制約をNative V2へ強制しない。

## 12. Regression checklist

旧資産parser / converterを変更したら最低限確認:

- 0..45がTerrainへ入る
- 46..50がV1同様Empty
- -1の最後のspawnが採用される
- -2..-6がEnemy spawnへ変換される
- 100/110/301/321..325/-79がTerrainで有効化されない
- 未解釈値が失われない
- visual gridのサイズ不一致を拒否する
- map/imgのサイズ・tile size前提を壊していない

CharacterController/terrain変更時:

- Solid
- OneWay
- DropThrough
- 2x1 slope
- 1x2 slope
- Ladder
- Water
- GravityUp/Down
- crushing

の既存テストを回す。
