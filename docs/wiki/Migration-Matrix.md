# HSP → Version1 → Version2 移植対応表

2026-09-21時点。

| 機能 | HSP | Version1 | Version2 |
|---|---|---|---|
| 通常Solid | yuka値 | Block ID 1 | Foundation TileDefinition実装済み |
| 見た目 | map値に混在 | img*.aryへ分離 | Visual layerあり |
| 坂 | 直接座標補正 | 独自実装/世代差あり | CanvasMasao基準で再構築済み |
| 2x1 slope | 旧処理 | 不完全/世代差 | Foundation実装済み |
| 1x2 slope | 旧処理 | 不完全/世代差 | Foundation実装済み |
| Coin | 負数床/Item | ID 3 | Foundation実装済み |
| HealingCoin | 旧Item | ID 23 | Foundation実装済み |
| OneUPCoin | 旧Item | ID 25 | Foundation実装済み |
| ? block | 負数床 | ID 4等 | Foundation実装済み |
| 10 coin block | 旧ギミック | ID 5 | Foundation実装済み |
| Brick | -70等 | ID 2 | BrickSystem実装済み |
| Ladder | -20 | ID 10 | Foundation実装済み |
| Ladder jump | あり | あり | Foundation実装済み |
| Ladder maker | あり | ID 8/28 | ItemSystem含め実装済み |
| Water | -230.. | ID 22 | Foundation実装済み |
| Gravity reverse | 旧特殊床 | ID 32/33 | Foundation実装済み |
| ON/OFF | あり | ID 21/38/39 | Foundation実装済み |
| 時間出現/消滅 | あり | ID 34/35 | Foundation実装済み |
| Coin条件地形 | あり | ID 29/36/37等 | ConditionalTerrain実装済み |
| Damage floor | あり | ID 40.. | Foundation実装済み |
| Instant death | あり | ID 40..のKill値等 | Foundation実装済み |
| OneWay | あり | Cloud/Through | Foundation実装済み |
| Drop-through | あり | Through | Foundation実装済み |
| Pipe animation | あり | あり | PipeTransport実装済み |
| Pipe destination | mov+coo+inf | StageMoving+AppearData | 直接PipeLinkへ簡単化 |
| 別submap移動 | あり | あり | 本編接続は未実装 |
| 通常Goal | item 301 | BeatItem ID 30 | GoalKind::Normal実装済み |
| 裏Goal | item 326 | BeatItem2 ID 31相当 | GoalKind::Secret実装済み |
| 中間地点 | item 325 | 正規データに値残存、runtimeでは未対応 | 未実装 |
| 順番1UP | 321..324 | 値残存、runtimeでは未対応 | 未実装 |
| Star Coin | -79 | 値残存、runtimeでは未対応 | 未実装 |
| Enemy配置 | 501..600 | map -2..-6 | ObjectSpawn構造へ移行予定 |
| Enemy runtime | enemy.hsp | ObjectManager | 未移植 |
| Lift / moving platform | giz | 旧Object/Gimmick | 未移植 |
| Boss | boss*.hsp | 旧runtime | 未移植 |
| Player物理 | scharamoving.hsp | Player.cpp | CharacterControllerあり。本編未接続 |
| Tile効果 | yuka_obj等直書き | Block派生クラス | TileRule/TileEffectへ分離済み |
| Score/HP/Coin adapter | 直書き | Manager直呼び | Foundation→本編adapter未実装 |
| Save | savedata.save | SaveData.dat | StageProgressあり、Save統合未実装 |
| WorldMap | wmcstage | BeatLevel | StageProgressへの移行未実装 |
| 通常/裏clear履歴 | STClearFlag等 | BeatLevelで区別消失 | StageClearStateで分離済み |
| Shooting | sshoot.hsp等 | 一部/別系統 | 未移植 |
| STG mode | あり | 別実装/限定 | 未移植 |

## ステータスの読み方

### Foundation実装済み

単体機能・テストは存在するが、必ずしも本編Actionから使われているとは限りません。

### 本編未接続

Version2の最重要残件。

特にPlayer、Map load、Object、Save、WorldMapは旧runtimeとの橋渡しが必要です。

## HSP限定機能

以下は「昔存在したことが確認できた」だけで、Version2の必須要件ではありません。

- Star Coin
- 中間地点
- 321..324 順番取得1UP
- HSPのサブマップファイル構造
- shooting/STG系
- 一部特殊Enemy/Gimmick/Boss

復活する場合は旧コードをそのまま戻さず、Version2の型・状態管理へ再設計します。
