# Version1 WorldMapデータ仕様

C++ Version1の `WorldMap.cpp` / `StageSelect.cpp` と、実データ

- `dat/wmap/wmblock.ary`
- `dat/wmap/wmlay.ary`
- `dat/wmap/event/501.ary`
- `dat/wmap/event/502.ary`
- `dat/wmap/event/999.ary`

を照合したメモです。

## 1. WorldMap読込

`StageSelect` はWorldMapを次のように生成します。

```cpp
Games = new WorldMap(
    "dat/wmap/wmblock.ary",
    "dat/wmap/wmlay.ary");
```

`wmblock.ary` が挙動・イベント・通行状態、
`wmlay.ary` が見た目を担当します。

これはAction stageの

```text
map*.ary
img*.ary
```

と似た分離です。

---

## 2. 基準データ

2026-09-21提供分。

| File | Size | Shape / Encoding | SHA-256 |
|---|---:|---|---|
| `wmblock.ary` | 3218 bytes | 20×32 / UTF-8 BOM | `1a90d4d5b29256320fa296cb34a7b6f1550ebd1bc7535196ac9bbb37eee2a78d` |
| `wmlay.ary` | 2578 bytes | 20×32 / UTF-8 BOM | `85b7a5c452a0d3ea050547944495598797c844085159f60b05d807e7d9363966` |
| `501.ary` | 20 bytes | CP932 | `8de2636df61fd36316bb678f9b3220f96e1210000b0acf7b47d72dc3c5297c46` |
| `502.ary` | 217 bytes | CP932 | `302c8cb08384637d7ad7684a0e9e3ef18a7a1e310a78094e84fa2545fe7cde21` |
| `999.ary` | 2017 bytes | CP932 | `07fa47be44171e6f933fe1aa75ea10bcadefdb5cac0b30ea35849ed417692711` |

---

## 3. wmblock.aryの値

確認された非0値:

- `1`
- `5`
- `6`
- `8`
- `9`
- `501`
- `999`
- `-5`
- `-6`
- `-8`
- `-9`
- `-500`
- `-502`

### 0

通行不可。

`WorldMap::IsBlock` は、

```cpp
Data <= 0 && Data > -500
```

をブロック扱いするため、0は通行不可。

### 1

通常通行可能セル。

### 正の5以上

Playerがそのセル上でZを押すと、

```cpp
KaiwaInit(Data[y][x]);
```

されます。

`KaiwaInit` はまず

```text
dat/wmap/event/<Num>.ary
```

を探し、

- event fileあり → 会話/イベント開始
- event fileなし → `LevelChanger::Change(Num)`

となります。

したがって同じ数値空間で、

- Stage入口
- 会話ポイント
- Save point

を表現しています。

### 負の -1..-499

初回ロード時に、

```cpp
if (BeatLevel[-value] == 1)
    Data = 1;
```

となります。

つまり、

```text
-5
-6
-8
-9
```

は対応ステージのクリア状態で開通する通路セル。

未クリア時は `IsBlock` により通行不可。

### -500以下

Playerが乗ると自動イベントとして処理されます。

```cpp
if (Data <= -500) {
    id = -Data;
    ...
}
```

event fileが存在すれば一度だけ会話を開始し、同時に `BeatLevel[id]` を立ててセルを1へ変更します。

event fileが無い場合でも、そのIDを完了扱いにしてセルを1へ変えます。

---

## 4. 実データ配置

`wmblock.ary` は20×32。

主な特殊セル:

| x | y | value | 意味 |
|---:|---:|---:|---|
| 2 | 2 | 999 | 初期位置 + intro event |
| 4 | 2 | 501 | Save event |
| 7 | 2 | 5 | Stage/Event 5 |
| 8 | 2 | -5 | Stage 5 clearで開通 |
| 9 | 2 | -500 | auto event/state 500 |
| 14 | 4 | 6 | Stage/Event 6 |
| 14 | 5 | -6 | Stage 6 clearで開通 |
| 11 | 8 | -8 | Stage 8 clearで開通 |
| 12 | 8 | 8 | Stage/Event 8 |
| 7 | 9 | 9 | Stage/Event 9 |
| 7 | 10 | -9 | Stage 9 clearで開通 |
| 7 | 12 | -502 | one-shot auto event 502 |

---

## 5. 999の特殊処理

Constructorで、

```cpp
if (Data[i][j] == 999) {
    Data[i][j] = -999;
    pos.x = j * 32;
    pos.y = i * 32;
}
```

と処理されます。

つまり `999` は、

1. WorldMap初期位置を決定
2. 内部値を `-999` に変換
3. `-500以下` の自動イベント条件に入る
4. `event/999.ary` を発火

という二重用途。

実際の `999.ary` はゲーム導入会話・操作説明を含む長い会話データ。

これにより、**ゲーム開始地点へ置くだけで初回導入イベントが発火する**設計になっています。

---

## 6. 501.ary

内容:

```text
Save
1_0,セーブ終了
```

`WorldMap::KaiwaEv` では、現在行の先頭が `Save` の場合、

```cpp
GameData::GetInstance().DataSave();
```

を実行して次行へ進みます。

したがって `wmblock=501` は、event fileを利用したSave point。

WorldMap側に「501はSave」というハードコードはありません。

**特殊命令をevent script側へ持たせている**点が重要。

---

## 7. 502.ary

通常の会話イベント。

`wmblock` 側には `-502` が置かれているため、

- Playerが到達すると自動発火
- 一度発火すると `BeatLevel[502]` を立てる
- map cellを1へ変更
- 次回以降は発火しない

というone-shot auto dialogueになっています。

---

## 8. Event script形式

確認できる通常行:

```text
FaceId,Text1,Text2,...
```

例:

```text
Yuri_0,体験版ってなんだよ・・・
Ayu_3,結構ここまで作るのにも,時間かかってるからな
```

`KaiwaDraw` は、

```text
dat/img/face/<FaceId>.bmp
```

を顔画像として読み、後続列を複数行の本文として描画します。

確認できる命令:

### Save

現在行が `Save` の場合SaveDataを書き出す。

### Start

会話送り後、次行先頭が `Start` の場合、

```cpp
LevelChanger::Change(KaiwaNum);
```

して、そのevent IDと同番号のStageへ入る。

したがってevent fileをStage前会話として使い、その末尾でStageへ遷移することも可能。

---

## 9. wmlay.ary

20×32で `wmblock.ary` と同サイズ。

確認値:

- 0
- 1
- 2
- 6
- 8
- 9
- 10
- 15
- 18
- 19
- 80

`WorldMap::draw` は各セルについて、

```cpp
DrawImg(..., WMAPTILE, ImgData[i][j], FALSE);
```

するため、純粋なVisual tile index。

`WMLayer.bmp` の画像番号と対応する。

---

## 10. WorldMap描画上のStage marker

`wmblock` の値が、

```text
5 <= value < 500
```

なら、WorldMap上でStage point iconを描画する。

- 未クリア → WMAPPOINT image 0
- clear済み → WMAPPOINT image 1

一方、

```text
-500 < value < 0
```

ならlocked route iconとして WMAPPOINT image 7 を描画する。

---

## 11. Version2移植で残すべき意味

V1の数値ルールをそのままVersion2へ持ち込む必要はない。

意味として分離する。

```text
WorldMap
 ├─ VisualTileMap
 ├─ Walkable/Route data
 ├─ StageNode[]
 ├─ EventNode[]
 ├─ UnlockCondition[]
 ├─ AutoEvent[]
 └─ StartPosition
```

### StageProgressとの対応

V1:

```text
BeatLevel[id] == 1
```

Version2:

```text
StageProgress / EventProgress
```

へ意味を分けるべき。

特にV1では同じ `BeatLevel` 配列を、

- Stage clear
- WorldMap auto event実行済み
- route unlock
- 会話event済み

に共用している。

Version2ではこれらを必要に応じて別状態へ分離した方がよい。

---

## 12. 重要な設計上の発見

今回の実データから、Version1 WorldMapは

```text
wmblock.ary = logic/event/state marker
wmlay.ary   = visual
event/*.ary = dialogue/command script
```

という3層に既に分離されていることが確認できた。

Action stageで検討している、

```text
Terrain
Visual
Object/Event
```

の分離とも方向性が近い。

Version2のWorldMap移植でも、数値セルの意味を直接復活させるより、各責務へ分解する方針が適している。
