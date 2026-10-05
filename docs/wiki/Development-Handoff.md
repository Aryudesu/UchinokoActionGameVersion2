# 開発引き継ぎ・新規スレッド開始ガイド

このページは、別スレッド・別セッションからVersion2移植作業を再開するときの入口です。

## 1. 最初に確認するもの

原則として次の順に確認します。

1. `dev` の最新状態
2. Open PR一覧
3. PR #29 のWiki草案
4. 実装中PRのdiff
5. 必要に応じてVersion1/HSPの実装

### 現在の資料PR

- PR #29: **Wiki草案・移植履歴の保管用。マージしない**
- PR #28: Version1 ARY互換ローダー。実装PR

PR番号や状態は将来変わるため、新しいスレッドでは必ず最新状態をGitHubから再確認すること。

## 2. 新規スレッドでの再開指示例

```text
UchinokoActionGameVersion2移植の続きです。
dev、open PR、特に資料用PR #29を確認して現在地を把握してから続けてください。
Version1互換とHSP旧仕様を混同しないようにしてください。
```

この指示があれば、過去チャット履歴だけに依存せずリポジトリ側から現在地を復元できます。

## 3. 状況把握時に確認するポイント

- 何が `dev` へmerge済みか
- 何がopen PR上だけにあるか
- Foundation実装済みか
- 本編runtimeへ接続済みか
- Legacy compatibility用か
- V2 native用か
- HSPにだけ存在した仕様か
- 意図的に復活させない仕様か

特に「Foundation実装済み」と「ゲーム本編で使用中」は別です。

## 4. 実装前に読むページ

### 地形・Player

- [Version2仕様](Version2-Spec.md)
- [設計判断ログ](Architecture-Decisions.md)

### V1ステージ取込

- [Version1仕様](Version1-Spec.md)
- [互換性の不変条件](Compatibility-Invariants.md)
- [Stage 5 実データ対応例](Reference-Stage5.md)

### HSP旧機能の調査

- [HSP版仕様](HSP-Spec.md)
- [移植対応表](Migration-Matrix.md)

### 次に何をするか

- [残件・未移植・設計判断](Remaining-Work.md)

## 5. 新しい事実が見つかったとき

次の3段階を分けて記録します。

1. **Historical fact**
   - HSP/V1で実際にどう動いていたか
2. **Compatibility requirement**
   - 既存V1資産を読むために何を再現する必要があるか
3. **V2 design**
   - Version2で今後どう実装するか

この3つを混ぜないこと。

## 6. 実装PRを作るとき

PR本文には最低限、

- 何を移植したか
- HSP/V1のどの仕様を参照したか
- V2では何を変えたか
- 互換性をどこまで保証するか
- 未対応として残したもの
- テストした不変条件

を書く。

## 7. 文字コード

旧HSPファイルにはCP932由来のものがあります。

Version2 C++ソースでは、日本語コメントを追加したファイルがVisual Studioのcode page 932で警告 `C4819` になることがあります。

新規・更新するC++ソースは、既存プロジェクト設定とVisual Studioで文字化けしないエンコーディングを確認すること。

特に自動生成・API経由で日本語コメントを追加したファイルはビルド前に確認する。


## 8. 2026-09-27 現在地

devへmerge済みのEnemy系はPR #63まで。

- #47 BallSlime / BallSlime2
- #48 ProjectileSystem / StationaryShooter / Pikachii
- #49 Chikorarashi / Ballistic
- #50 Bounce / Split projectile
- #51〜53 FlyingEnemy 4 variants
- #54 Kameen
- #55 FishEnemy 35〜37
- #56 WallCrawler 38/39
- #57 SeaAnemone 40/41
- #58 Mariri 34
- #59 TransformingWalker 26/27
- #60 UnstompableWalker 28
- #61 BulletEnemy 3
- #62 JumpingEnemy 8/9
- #63 PipeEnemy 18

今回優先していたHSP Enemy 3 / 8 / 9 / 18 / 28はすべてmerge・実機確認済み。PR #63 PipeEnemyでは、近距離待機、timer>50で接触/stomp有効、timer=100で上方飛び出し、着地後待機復帰まで確認済み。

Enemy追加フェーズはいったん区切る。次はEnemy死亡演出 / Score / SEなどの共通runtime effect、またはLift / moving platform基盤を優先する。16 / 17の線移動EnemyはLift / path movement基盤の後で扱う。

PR #64 / #65でMovingPlatform基盤もdevへmerge済み。

- #64: 1〜5マス幅、上面Stand、下からすり抜け、Lift上jump
- #65: `pathDelta + speed` の水平/垂直往復、`FrameDelta` によるPlayer carry
- P1〜P5 fixtureで実機確認済み
- HSP版1マス幅は `widthTiles=1` で表現可能

HSP Lift再調査では、`gizf=3/4` が左右/上下Lift、`5/6` が乗ると落下/上昇、`15..23` が乗ると起動するtimer/state列、`49→50` がguide tileに沿う線追従床と判明した。

特に `gizf=49/50` は現行の単一直線 `pathDelta` では表現しない。HSPの負数guide tileをruntimeへ復活させず、将来PathFollower / waypoint形式へ変換する。enemyf=16/17の線移動Enemyもmovement部分は同じPathFollower共有候補。

今後、未移植Enemyへ戻る場合は次を優先する。

1. HSPの実コードからupdate順序まで確認
2. 既存Walking/Flying/Projectile resolverを再利用
3. numeric enemyfをTypeIdにしない
4. state違いはBehaviorStateへまとめる
5. 左右・時計回り等の対称差はDirection/variantへまとめる
6. Camera lifecycleとの関係を明示する
7. 専用test Areaを既存の確認chainの後ろへつなぐ
8. Playerが次のAreaへ進める通路をfixtureで塞がない
9. Playerより高い位置に確認対象Enemyを置く場合は、通常ジャンプで到達できる地面から1〜2段程度を目安にする

特にWallCrawlerは、HSPの方向別if列挙ではなく、
4方向vectorと90度回転によるwall followerとして実装している。
