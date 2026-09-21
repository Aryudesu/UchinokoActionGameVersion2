# UchinokoActionGame 仕様Wiki

このWikiは、**HSP版 → C++ Version1 → Version2** の仕様・移植経緯・現状・残件を後から追えるようにするための資料です。

## ページ一覧

- [HSP版仕様](HSP-Spec.md)
- [Version1仕様](Version1-Spec.md)
- [Version2仕様](Version2-Spec.md)
- [HSP → V1 → V2 移植対応表](Migration-Matrix.md)
- [残件・未移植・設計判断](Remaining-Work.md)
- [Version2 ステージデータ設計案](Stage-Data-Design.md)
- [開発引き継ぎ・新規スレッド開始ガイド](Development-Handoff.md)
- [互換性の不変条件](Compatibility-Invariants.md)
- [Stage 5 実データ対応例](Reference-Stage5.md)
- [設計判断ログ](Architecture-Decisions.md)
- [移植PR履歴](Migration-PR-History.md)
- [画像アセット対応メモ](Asset-Inventory.md)

## 仕様を読むときの優先順位

### 地形・衝突

Version2では、地形・坂・当たり判定の基準を **CanvasMasao由来の仕様 + Version2 Foundation実装** に寄せています。

HSP版・Version1の座標補正や当たり判定コードは、旧作の挙動確認・操作感確認用の資料として扱います。

### 旧ステージデータ

C++ Version1の正規ステージデータは原則として、

- `Data{detail}.inf`
- `map{detail}.ary`
- `img{detail}.ary`

の組を基準とします。

HSP版の `map / mov / coo / inf` は、Version1へ移植される前の旧仕様です。

### 古い機能

HSP版に存在した機能を見つけても、**Version2へ自動的に復活させる要件にはしません**。

区別して記録します。

1. HSPで存在した
2. Version1でも存在した
3. Version2で実装済み
4. Version2で再設計した
5. 現在は未移植
6. 意図的に復活させない

## 大きな変遷

```text
HSP
  1枚のmapに
  地形 / 見た目 / Item / Gimmick / Enemy / Spawnを混在
        ↓
C++ Version1
  map*.ary       挙動
  img*.ary       見た目
  Data*.inf      ステージ設定・サブマップ移動
        ↓
Version2
  Foundationで役割分離
  Terrain / Visual / Object / Event
  + CharacterController
  + TileDefinition / TileInteraction
  + PipeLink
  + StageProgress
```

Version2新規データについては、さらに **Terrain / Visualはタイルグリッド、Enemy・Lift等は座標付きObject配置リスト** とする方向を検討しています。

## 現在の重要な注意点

Version2のFoundationには多くの機能が移植済みですが、**本編の `Action / Map / PlayerManager` はまだVersion1由来runtimeが中心**です。

したがって、

- Foundationに実装済み
- 実ゲームruntimeへ統合済み

は同義ではありません。

各ページでは可能な限りこの2つを分けて記載します。

## 関連PR

- PR #28: Version1 ARYステージをLayeredMapへ取り込む互換ローダー
- PR #29: このWiki内容の調査・移植履歴を保持する資料用Draft PR


## 新しいスレッドから再開するとき

まず [開発引き継ぎ・新規スレッド開始ガイド](Development-Handoff.md) を確認し、その時点の `dev` とOpen PRを再取得してください。

PR #29は履歴・Wiki草案の入口ですが、実装状況そのものは常に最新の `dev` / Open PRを優先します。

## 互換実装を触る前に

[互換性の不変条件](Compatibility-Invariants.md) を確認してください。

特に、

- HSPの意味が分かっていてもV1で無効なら勝手に有効化しない
- Legacy V1とV2 nativeを分ける
- Player spawnはV1のrow-major上書き挙動を維持
- V1 block IDとV2正式IDを同一視しない

を重要な前提とします。
