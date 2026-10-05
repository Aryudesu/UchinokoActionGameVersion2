# Stage 5 実データ対応例

## 0. 基準fixture

2026-09-21に提供されたC++ Version1 Stage 5一式を、今後のLegacy importer / Data.inf loader検証の基準fixtureとして扱う。

| File | Size | Shape | SHA-256 |
|---|---:|---|---|
| `Data0.inf` | 150 bytes | - | `d74dfd6e3fbe9aa4ce89ccf36133b514d005d38f9260d8d0ea946ddc680b7801` |
| `Data1.inf` | 153 bytes | - | `0a3417c65e15a507af315a89e4de98231ebe785ca76695a6eb7ffa671742bd1f` |
| `map0.ary` | 27028 bytes | 30×180 | `2299926a9e0810292f76f07cf1f9b51d8b8444053c80d8ba78ca785e21f8461f` |
| `img0.ary` | 27028 bytes | 30×180 | `effcb81e142f15b0a3b2c4aa09d4b30382184d86c80251be45f6fd5bb3159bde` |
| `map1.ary` | 648 bytes | 10×16 | `bcf74fbebe4b145002fa52d6a26395b4c766349ec0d78df3916ecc100fcaaeec` |
| `img1.ary` | 649 bytes | 10×16 | `ee07ff7972cb3929bdd05e1b766fcc0ba25c5a59f95308aa3c062a328b5bc26c` |

`Data0.inf` / `Data1.inf` はUTF-8 BOM付きで確認。

同梱の `Log.txt` はDxLib終了ログであり、Stage definition / runtime assetではないためfixture対象外。

### Data0.inf

```ini
>StageData
GameMode = 0
Time = 300
ScrollMode = 0
StageMoving = 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0
>AppearData
1 = 1,97.5,22
>BGMData
BGM = 1
```

### Data1.inf

```ini
>StageData
Time = 0
ScrollMode = 4
StageAppear = 0
StageMoving = 0,0
>ImgDat
Background = haikei3.bmp
Background2 = haikei3.bmp
>BGMData
BGM = 2
```

このfixtureを使い、将来のloaderテストでは少なくとも、

- detail 0 / detail 1 のサイズ差
- optional key / section
- `StageMoving`
- `AppearData`
- `StageAppear`
- `ImgDat` override
- BGM
- V1で無効なHSP残存marker
- Player / Enemy marker

を確認する。


HSP → C++ Version1の変換を実データで確認した基準ステージです。

このページは「旧データがなぜこうなっているか」を調べる際のanchorとして使います。

## 1. HSP側

確認した主なファイル:

```text
map/map5-0.ary
map/map5-1.ary
mov/mov5-0.ary
mov/mov5-1.ary
coo/coo5-0-1.ary
coo/coo5-1-0.ary
inf/inf5-0.ary
inf/inf5-1.ary
```

`map5-0.ary` は30×180。

HSPでは1枚のmapへ、

- terrain
- image pattern
- Player spawn
- Item
- Gimmick
- Enemy

等が混在している。

## 2. C++ Version1側

```text
dat/stage/5/
  Data0.inf
  map0.ary
  img0.ary

  Data1.inf
  map1.ary
  img1.ary
```

### detail 0

- `map0.ary`: 30×180
- `img0.ary`: 30×180

### detail 1

- `map1.ary`: 10×16
- `img1.ary`: 10×16

## 3. HSP map5-0 → V1 map0/img0

実データ上、HSPの1値がV1で「挙動」と「見た目」に分離された痕跡がある。

代表例:

| HSP | Version1 map | Version1 img | 解釈 |
|---:|---:|---:|---|
| 3/4/5/... | 1 | HSP側値 | Solid + 見た目分離 |
| -60 | 3 | 対応img | Coin |
| -70 | 2 | 対応img | Brick |
| -40/-41/-42等 | ItemBlock系 | 対応img | ? block系 |
| 100 | -1 | - | Player spawn |
| 502等 | -3等 | - | Enemy markerへ整理 |

これはランタイム変換ではなく、HSP→C++ V1への移植時に資産を書き換えたものと考えるのが自然。

## 4. V1 map0に残ったHSP値

正規V1 `map0.ary` にはHSP由来の値が一部残っている。

確認例:

- `110`
- `321`
- `322`
- `323`
- `324`
- `325`
- `-79`

ただしV1 runtimeではEmpty。

この事実は、Legacy importerでHSP意味を勝手に復活させない根拠になる。

## 5. サブマップ移動

### HSP detail 0

`mov5-0.ary`:

```text
0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0
```

X位置の区画ごとに移動先map番号を指定。

### HSP detail 1 → 0

`coo5-1-0.ary`:

```text
97,23
1
```

戻り先座標と退出方向を別ファイルで指定。

### Version1 Data0.inf

```ini
>StageData
GameMode = 0
Time = 300
ScrollMode = 0
StageMoving = 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0
>AppearData
1 = 1,97.5,22
>BGMData
BGM = 1
```

HSPの `mov` が `StageMoving`、`coo` が `AppearData` へまとめられている。

### Version1 Data1.inf

```ini
>StageData
Time = 0
ScrollMode = 4
StageAppear = 0
StageMoving = 0,0
>ImgDat
Background = haikei3.bmp
Background2 = haikei3.bmp
>BGMData
BGM = 2
```

サブマップごとの背景・BGM等はData*.infへ整理されている。

## 6. このサンプルから得られた結論

```text
HSP
 map + mov + coo + inf
       ↓ asset migration
C++ V1
 map + img + Data.inf
       ↓ compatibility import
Version2
 Terrain + Visual + Object/Event + Settings/Transition
```

HSP→V1の変換をVersion1 runtime内部に探さないこと。

Version2で旧HSP形式へ戻さないこと。
