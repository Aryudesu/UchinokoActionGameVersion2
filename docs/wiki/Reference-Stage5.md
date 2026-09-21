# Stage 5 実データ対応例

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
