# 互換性の不変条件

Legacy importer / Version1互換処理を変更するときに壊してはいけない条件です。

## 1. 「HSP互換」と「C++ Version1互換」を混同しない

Version2のLegacy importerが対象とする正規資産は **C++ Version1**。

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

Legacy importerもこれを保持する。

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

Legacy importではTerrainとVisualを混同しない。

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

## 11. LegacyとNativeを分ける

### Legacy

V1資産をなるべく忠実に取り込む。

### Native V2

新設計を使う。

Legacyの制約をNative V2へ強制しない。

## 12. Regression checklist

Legacy importerを変更したら最低限確認:

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
