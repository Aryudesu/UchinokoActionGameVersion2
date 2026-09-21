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
