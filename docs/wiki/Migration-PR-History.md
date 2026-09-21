# 移植PR履歴

Version2のFoundation移植がどの順番で進んだかを追うための索引です。

> PRの状態はこのページ作成時点の記録です。最新状態はGitHub上で再確認してください。

## Foundation移植シリーズ

| PR | 内容 | 記録時状態 |
|---:|---|---|
| #8 | ブロック・坂の地形判定をCanvasMasao方式へ再構成 | merged |
| #9 | 2×1坂を64×32の連続地形として再構成 | merged |
| #10 | 1×2坂を32×64の連続地形として再構成 | merged |
| #11 | Foundationへデータ駆動のタイルギミック基盤を追加 | merged |
| #12 | ？ブロックと飛び出しアイテムをFoundationへ実装 | merged |
| #13 | 10コインブロックを汎用カウントルールで実装 | merged |
| #14 | ON/OFFブロックと圧死判定をFoundationへ実装 | merged |
| #15 | 時間で出現・消滅するブロックをWorldStateへ実装 | merged |
| #16 | コイン枚数条件ブロックを汎用ConditionalTerrainで実装 | merged |
| #17 | はしご移動モードとはしご生成ブロックをFoundationへ実装 | merged |
| #18 | 水中移動をFoundationへ実装しGimmick testを調整 | merged |
| #19 | 重力反転ギミックをCharacterControllerへ実装 | merged |
| #20 | はしご移動中のジャンプをV1互換で復元 | merged |
| #21 | シンプルな外部データ駆動の土管移動を実装 | merged |
| #22 | V1のThrough床をDropThroughOneWayとして移植 | merged |
| #23 | 通常ゴールと裏ゴールを区別するStageProgressを追加 | merged |
| #24 | 通常ゴールと裏ゴールを独立したクリア状態として実装 | merged |
| #25 | V1互換のレンガブロックをBrickSystemとして移植 | merged |
| #26 | V1の対象別ダメージ・即死ブロックを移植 | merged |
| #27 | V1の直接取得Coin・HealingCoin・OneUPCoinを移植 | merged |
| #28 | Version1 ARYステージをLayeredMapへ分解する移行parser | open / 実装PR |
| #29 | Wiki草案: HSP / Version1 / Version2 仕様・移植状況 | open draft / **DO NOT MERGE** |
| #30 | V2ネイティブStageData / Layer / Object / Regionモデル | merged |
| #31 | Native StageData JSON + CSV Loader | merged |
| #32 | Native StageData TileSet参照 + Sandbox描画 | open |

## 読み方

この表は単なる変更履歴ではなく、Version2の設計が固まった順番でもあります。

大きく分けると、

```text
#8-10
Terrain / slope

#11-20
Tile behavior / movement gimmick

#21-22
Transition / one-way

#23-24
Goal / persistent progress

#25-27
V1 block/item behavior

#28-29
Legacy調査・移行資料

#30
V2 native stage data / editor-ready model

#31
Native JSON + CSV data / runtime loading

#32-
TileSet / native rendering / gameplay bridge
```

という流れです。

## 新規スレッドでの利用

新しく作業を再開するとき、

1. このページで移植済み領域を確認
2. [残件](Remaining-Work.md) を確認
3. Open PRをGitHubから再取得
4. 実装対象に関係する過去PRのdiffを見る

とすると、Foundationに同じ機能を二重実装しにくくなります。

## 注意

「PRがmerged」は **Foundation側の部品がdevへ入った** ことを意味する場合があります。

本編 `Action / PlayerManager / ObjectManager / GameData` まで統合済みとは限りません。

この区別は [Version2仕様](Version2-Spec.md) と [残件](Remaining-Work.md) を参照してください。
