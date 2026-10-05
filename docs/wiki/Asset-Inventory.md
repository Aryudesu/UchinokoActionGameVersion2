# 画像アセット対応メモ

2026-09-21時点で確認できたHSP / Version1系画像アセットの整理です。

> 画像自体をWikiへ複製することが目的ではなく、
> 「どのファイルがどのruntime読込と対応するか」を後から追えるようにするための索引です。

## 1. Version1読込コードと一致するもの

`Action::LoadImg` / `WorldMap` / `ImageManager::LoadImg` と照合。

| File | Size | Version1側読込 | 備考 |
|---|---:|---|---|
| `item.bmp` | 128×128 | `LoadImg(ITEM, 4, 4, ...)` | 32×32を4×4。完全一致 |
| `effect.bmp` 相当 | 320×704 | `LoadImg(EFFECT, 10, 22, ...)` | 32×32を10×22。完全一致 |
| `mapobj.bmp` | 320×704 | `LoadImg(MAPOBJ, 10, 22, ...)` | 32×32を10×22。完全一致 |
| `object.bmp` | 320×384 | `LoadImg(OBJECT, 10, 12, ...)` | 32×32を10×12。完全一致 |
| `score.bmp` | 160×32 | `LoadImg(SCORE, 16, 16, 10, 2, ...)` | 16×16を10×2。完全一致 |
| `WMPoint.bmp` | 352×32 | `LoadImg(WMAPPOINT, 10, 1, ...)` | 32×32を10個だけ読むため、画像には余剰領域がある |
| `WMLayer.bmp` | 288×906 | `LoadImg(WMAPTILE, 9, 25, ...)` | 32×32を9×25読む。画像全高には余剰領域がある |

### 注意

`WMPoint.bmp` は幅352px = 32px × 11ですが、Version1の現行コードは10個しか分割ロードしません。

`WMLayer.bmp` も、9×25×32pxで読まれる領域は288×800pxであり、実ファイル全高906pxより小さいです。

したがって画像内に「現行V1 runtimeでは使用しない旧素材・余剰素材」が残っている可能性があります。

これらを見つけても、画像内に存在するだけでVersion2へ必要な機能と判断しないこと。

---

## 2. HSP時代・別モード系として有用なもの

今回確認した以下のファイルは、名称・内容からHSP時代や別モード系の仕様調査に有用。

- `item2.bmp`
- `giz.bmp`
- `giz2.bmp`
- `eshoot.bmp`
- `stgenemy.bmp`
- `wmap.bmp`

### giz / giz2

特殊床・ギミック用と思われるスプライトがまとまっており、HSP `giz.hsp` や `yuka_obj.hsp` と照合する資料として使用できる。

### eshoot

Enemy shot系の画像資産。

HSP `enemyshoot.hsp` / STG系実装の調査時に使用する。

### stgenemy

STG系Enemy画像。

通常Action側のVersion1 `enemy.bmp` とは別物として扱う。

### item2

HSP側・別世代Item画像の対応確認用。

---

## 3. 現時点で特に追加で有用な画像

必須ではないが、今後の移植であると助かるもの。

### `enemy.bmp`

確認済み。

実ファイル:

- 384×444 px
- Version1は `LoadImg(ENEMY, 12, 7, ...)`
- 32×32を12列×7行として、先頭384×224pxを使用

現在のVersion1 `EnemyFactory` / `Enemy.cpp` と照合すると、map markerからsprite rowまで次のように対応する。

| map marker | EnemyKind | Class | BaseImg | sprite row |
|---:|---:|---|---:|---:|
| `-2` | 1 | `WalkingEnemy1` | 0 | 0 |
| `-3` | 2 | `WalkingEnemy2` | 12 | 1 |
| `-4` | 3 | `CarrotMan` | 24 | 2 |
| `-5` | 4 | `BallSlime` | 36 | 3 |
| `-6` | 5 | `BallSlime2` | 48 | 4 |

`Charactor::draw` は `BaseImg + image` を `ENEMY` sheetから描画する。

画像上でも先頭5行にそれぞれ敵spriteが並んでおり、現在の5種類と整合する。

7行分ロードするが、現行5Enemyが使用するのは主にrow 0..4。
row 5..6は現行コード上ではBaseImgとして参照されていない。

また実ファイルの高さ444pxに対し、Version1が分割ロードする範囲は224pxまでであり、それ以降は現行runtimeでは使用しない。確認した範囲では下部は背景色のみ。

このため、`enemy.bmp` については **map -2..-6 → EnemyKind 1..5 → sprite row 0..4** を正規V1対応として扱える。

### `maincharactor.bmp`

Playerを `CharacterController` へ接続した後、旧アニメーション状態と描画番号を確認する際に使用。

### Background系

本編StageSettings / Data.inf統合時に必要。

ただし、Foundation stage loader・Legacy importer・TileCatalog mappingの作業には必須ではない。

---

## 4. 画像アセットを読むときの方針

1. **Version1 runtimeが実際にロードしているファイル・分割数を優先**
2. HSP資産は旧仕様調査用
3. 画像内に未使用spriteがあっても「機能が必要」とは限らない
4. Tile ID / Object ID / Image indexを同一概念とみなさない
5. Version2ではBehaviorとVisualを分離する

特に、

```text
map code
object kind
image index
animation frame
```

は別の番号体系として扱う。

---

## 5. Version2での利用

### Terrain / Visual

`img*.ary` とBlock画像sheetの対応確認。

### ItemSystem

`item.bmp` を使ってV1 ItemKindの描画frameを確認。

### ObjectSpawn / Enemy

`enemy.bmp` とObject実装を照合予定。

### Lift / Moving Object

`object.bmp`, `mapobj.bmp`, HSP `giz*.bmp` を参照しながら、旧Object番号と新 `ObjectSpawn` の意味を分離する。

### WorldMap

`WMPoint.bmp`, `WMLayer.bmp` はWorldMapをStageProgressへ接続する段階で参照する。
