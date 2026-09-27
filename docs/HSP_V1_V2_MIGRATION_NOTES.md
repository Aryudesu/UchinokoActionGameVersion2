# HSP版 → Version1 → Version2 移植メモ

> **この文書は移植履歴・旧仕様・未移植機能・設計判断を後から追えるようにするための作業メモです。**
>
> 現時点では確定仕様書ではありません。HSP版の実ファイル、C++ Version1の実装・データ、Version2 Foundation の実装を照合して分かったことを記録します。
>
> 特に、HSP版の旧仕様をすべてVersion2へ復活させることは目的ではありません。Version2では単純化・再設計した仕様を優先し、旧データ互換が必要な箇所だけ明示的に変換します。

---

## 1. 調査に使った資料

### HSP版

主に以下を確認。

- `MAIN2.hsp`
- `loadmap.hsp`
- `function.hsp`
- `yuka_obj.hsp`
- `scharamoving.hsp`
- `itemobj.hsp`
- `giz.hsp`
- `enemy.hsp`
- `enemyshoot.hsp`
- `sshoot.hsp`
- `sal.hsp`
- `loadwmap.hsp`
- `map/map5-0.ary`
- `mov/mov5-0.ary`
- `mov/mov5-1.ary`
- `coo/coo5-0-1.ary`
- `coo/coo5-1-0.ary`
- `inf/inf5-0.ary`
- `inf/inf5-1.ary`

### C++ Version1

特に以下を確認。

- `Map.cpp`
- `BlockFactory.cpp`
- `Action.cpp`
- `Player.cpp`
- `GameData.cpp`
- `WorldMap.cpp`
- `dat/Memo.txt`
- `dat/仕様.txt`
- `dat/stage/5/Data0.inf`
- `dat/stage/5/Data1.inf`
- `dat/stage/5/map0.ary`
- `dat/stage/5/img0.ary`
- `dat/stage/5/map1.ary`
- `dat/stage/5/img1.ary`

### Version2

Foundation以下の実装と、これまでの移植PRを参照。

---

## 2. ステージデータ構造の変遷

### 2.1 HSP版

HSP版では、基本的に **1枚のマップ配列に地形・開始位置・アイテム・特殊床・敵などを混在** させていた。

`loadmap.hsp` では代表的に次の範囲を特別扱いしている。

| HSPマップ値 | 意味 |
|---:|---|
| `100` | 通常開始位置 |
| `110` | 中間開始位置 |
| `301..400` | Item。内部では `itemf = code - 300` |
| `401..500` | 特殊床/Gimmick。内部では `gizf = code - 400` |
| `501..600` | Enemy。内部では `enemyf = code - 500` |
| `-79` | スターコイン関連 |

その他、負数側にもコイン、レンガ、ON/OFF、ダメージ床、通り抜け床、水など多数の床・ギミックが割り当てられていた。

この形式では「そのセルの値は1個」なので、**同一セルに地形と敵、地形とリフト、見た目違いの地形とオブジェクトなどを独立に置くことが難しい**。

### 2.2 C++ Version1

C++ Version1への移植時点で、HSPの1枚構成はかなり整理されている。

代表的な分離:

- `map*.ary`: 挙動・ブロック・一部スポーン
- `img*.ary`: 見た目
- `Data*.inf`: 時間、スクロール、サブマップ移動、背景、BGM等

Stage 5 のHSP `map5-0.ary` と C++ V1 `map0.ary / img0.ary` は同じ30×180であり、実データ上も次のような対応が見える。

| HSP側 | C++ V1側 |
|---|---|
| `3/4/5/... ` の地形画像値 | `map=1(Solid)` + `img=旧画像番号` |
| `-60` | `map=3(Coin)` |
| `-70` | `map=2(Brick)` |
| `100` | `map=-1(Player spawn)` |
| HSP敵コード | C++側では `-2..-6` の一部EnemyKindへ整理 |

つまり、**C++ Version1の実行時に毎回「HSPマップ→V1マップ変換」をしていたのではなく、HSP→C++移植時に資産そのものを整理・変換した**と考えるのが自然。

### 2.3 C++ Version1の未対応旧コード

C++ V1 `Map::LoadMap` の実行ルールは次の通り。

- `0..50`: `BlockFactory` へ
- `-1`: Player初期位置
- `-2..-6`: EnemyKind 1..5
- その他: Empty

現在の `BlockFactory` で実際に意味を持つのは主に `0..45`。

一方、正規C++ V1ステージデータにも以下のようなHSP時代の値が残っている。

- `110`
- `301`
- `321..325`
- `-79`

これらはHSP版では意味があったが、C++ V1ではこの読込経路上 **Empty扱い**。

Version2の互換ローダーでも、ゲーム挙動はC++ V1を正としてEmpty扱いにしつつ、元コードだけは調査用メタデータとして保持する方針。

---

## 3. HSP時代に確認できた旧機能

### 3.1 Item系

HSP `itemobj.hsp` で確認できる代表例。

| HSP map code | itemf | 意味 |
|---:|---:|---|
| `321` | 21 | 順番取得1 |
| `322` | 22 | 順番取得2 |
| `323` | 23 | 順番取得3 |
| `324` | 24 | 順番取得4。成立時1UP |
| `325` | 25 | 中間地点 |
| `326` | 26 | 裏ゴール |

スターコインも存在し、ワールドマップ進行状態と連動する処理があった。

### 3.2 床・ギミック系

`yuka_obj.hsp` / `giz.hsp` から、少なくとも次の系統が存在していた。

- はしご
- はしご中ジャンプ
- 音符ブロック
- 隠しブロック
- コイン
- 二段ジャンプリセット
- コイン減少 / リセット
- 水
- 回復
- スターコイン
- ?ブロック
- ツタ / はしご生成
- レンガ
- ON/OFF
- ダメージ床
- 即死床
- 踏むと壊れる床
- 一方通行 / 下抜け
- 一定周期で出現・消滅
- 射撃で消えるブロック
- コイン数条件地形
- 土管 / サブマップ移動
- リフト系
- その他特殊床

### 3.3 Player移動

`scharamoving.hsp` では旧作の操作感・物理が直接実装されている。

例:

- 水中ジャンプは通常の `2/3`
- 水中重力は通常の `1/3`
- はしご中は通常Y移動を抑制
- 坂は左右別の直接座標補正
- ジャンプキーを離すと上昇速度を減衰

Version2では地形衝突の正本を CanvasMasao 系ロジックへ寄せているため、HSPの座標補正をそのまま再実装する必要はない。

HSPコードは **旧作の操作感・意図を確認する資料** として使用する。

---

## 4. 土管・サブマップ移動の変遷

### 4.1 HSP版

HSP版では土管移動が複数ファイルに分散していた。

#### mov

`mov/mov5-0.ary`:

```text
0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0
```

PlayerのX座標を10タイル単位で区切り、その場所から入った場合の移動先サブマップ番号を決定する。

#### coo

移動元・移動先の組ごとに別ファイル。

例:

`coo/coo5-1-0.ary`

```text
97,23
1
```

- 1行目: 出現座標
- 2行目: 退出方向

`-1` の場合は座標を上書きしない。

#### inf

移動後、

```text
inf/inf<stage>-<map>.ary
```

を読み直し、

- mapファイル
- 背景
- map画像
- item画像
- gimmick画像
- enemy画像
- lift画像
- enemy shot画像
- 背景2
- effect画像
- 背景フラグ
- BGM
- 慣性
- boss mode
- scroll mode
- 暗闇
- 制限時間
- 水中

等をまとめて再設定する。

つまりHSP版では概念的に、

```text
入口
 ↓
現在X座標
 ↓
mov配列から移動先mapを決定
 ↓
coo(from,to)から出現座標・退出方向を決定
 ↓
inf(target)からステージ設定を再読込
 ↓
target mapをロード
```

という構成。

### 4.2 C++ Version1

C++ V1では既に一段簡単化されている。

Stage 5 `Data0.inf`:

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

HSPの、

- `mov` → `StageMoving`
- `coo` → `AppearData`
- `inf` → `Data*.inf`

へ統合されたと考えられる。

### 4.3 Version2

Version2 Foundationではさらに単純化して、入口と出口を直接結ぶ。

現状:

```cpp
struct PipeLink {
    WorldPosition EntryPosition;
    PipeDirection EnterDirection;
    WorldPosition ExitPosition;
    PipeDirection ExitDirection;
};
```

旧 `mov/coo/inf` をランタイムで再現する予定はない。

必要なら旧データ取込時だけ、

```text
StageMoving + AppearData
        ↓ import
PipeLink / StageTransition
```

へ変換する。

### 今後のサブマップ対応

現在の `PipeLink` は同一マップ内の入口・出口を直接結ぶ形。

別 `map1` 等への移動をVersion2で残す場合も、HSP/V1方式へ戻さず、

```cpp
TargetMapId
ExitPosition
ExitDirection
```

程度を追加する単純な明示リンク方式を推奨する。

---

## 5. Version2で実装済みの主な移植

2026-09時点。

### 地形・基本衝突

- Block / slope terrain を CanvasMasao ベースへ再構成
- 2x1 slope
- 1x2 slope
- data-driven tile gimmick Foundation

### ブロック・ギミック

- ? block + pop-out item
- 10 coin block
- ON/OFF block
- crushing
- timed appearing/disappearing block
- coin-count conditional terrain
- ladder
- ladder maker
- water
- gravity reverse
- ladder jump
- through / drop-through one-way
- BrickSystem
- damage / instant-death blocks
- direct collectible Coin / HealingCoin / OneUPCoin

### 土管

- 直接 `PipeLink` する簡易方式
- entering / fade out / fade in / emerging

### ゴール・進行

- normal goal / secret goal を独立したclear stateとして保持
- StageProgress

### 旧ARY

- Version1 `map*.ary / img*.ary` を `LayeredMap` へ分ける互換ローダーをPR #28で作業中

---

## 6. 現時点で未統合・未移植のもの

### 6.1 Foundationはあるが本編Actionへ未接続

大きな残件。

- `Action::LoadMapData` → 新Foundation stage loader
- Player → `CharacterController`
- TileEffect → PlayerManager / GameData等へのadapter
- GoalState / StageProgress → GameData / Save / WorldMap
- PipeTransport → 実ステージ間遷移
- LayeredMap → 本編描画・更新
- Object spawn → 新しいObject runtime

### 6.2 C++ V1に存在するが旧runtime側に残るもの

- 敵
- 動的Object
- moving gimmick / lift系
- ボス
- 演出
- score popup
- fragment
- SE/BGMとの連携
- WorldMap
- Save

### 6.3 HSP版に存在したがC++ V1では無効化・縮小されたもの

必要性を再判断する。

- `321..324` 順番取得1UP
- `325` 旧中間地点
- `-79` スターコイン
- HSP版の `326` 裏ゴールコードそのもの
  - Version2にはSecret Goalという概念は既に別実装済み
- HSP版の多数の敵ID / gimmick ID
- HSP版独自のサブマップ遷移データ構造
- HSP版のセーブ改ざん検査
- HSP版ワールドマップの細かい進行状態
- shooting / bomb 系
- STGモード系
- 一部ボス・特殊イベント

**「HSPにあったから全部戻す」はしない。**
必要なものだけVersion2の設計に合わせて再実装する。

---

## 7. Save / WorldMapの旧仕様メモ

HSP `sal.hsp` では少なくとも以下を保存。

- SHP
- ZANKI
- score
- coin
- world map X/Y
- `wmcstage[100]`

乱数キーと剰余/商を使った簡易整合性チェックも存在した。

`wmcstage` は単なるclear boolではなく、複数段階の進行状態として使われていた形跡がある。

Version2では既存 `StageProgress` を中心に再設計し、HSPの整数状態をそのまま復活させない。

---

## 8. Version2のステージ配置データ構造について

### 結論

**Terrain / Visual / Object を分離する方向にする。**

ただし、

> ObjectもTerrainと同じ2次元グリッドにする

のは将来制約になりやすい。

### 推奨

#### Terrain: グリッド

ブロック・衝突・MovementRegion等。

例:

```text
terrain.ary
```

タイル単位の配置なので2次元グリッドが自然。

#### Visual: グリッド

地形の見た目。

例:

```text
visual.ary
```

Terrainと同サイズ。

これにより、

- 同じSolidでも違う画像
- 装飾だけ変更
- collisionと画像の独立

が可能。

#### Objects: 配置リスト

敵、リフト、動く足場、動的ギミック等。

**2次元グリッドではなく座標付きリストを推奨。**

理由:

1. Terrainと同じ位置に置ける
2. 同じ位置に複数Objectを置ける
3. タイル中央以外へ配置できる
4. Objectごとに初期パラメータを持てる
5. 将来32px以外のサイズ・経路・速度等を自然に持てる

例:

```csv
type,id,x,y,param1,param2
Enemy,Walking,320,640,Left,
Enemy,Flying,480,400,,
Lift,Horizontal,800,600,range=192,speed=2
```

または将来的にJSON等へしてもよい。

#### Event / Trigger: 配置リストまたは領域

- goal
- checkpoint
- scene trigger
- camera trigger
- BGM trigger
- stage transition

等。

これも必ずしもタイルグリッドにする必要はない。

### LayeredMapの扱い

現在の `LayeredMap`:

- Terrain
- Visual
- Object
- Event

は **旧ARYを情報落ちなくFoundationへ運ぶ中間表現として有用**。

ただしVersion2新規ステージの最終設計としては、

```text
StageData
 ├─ Terrain TileMap
 ├─ Visual TileMap
 ├─ ObjectSpawn[]
 ├─ Event/Trigger[]
 ├─ Pipe/Transition[]
 └─ StageSettings
```

のように、Object/Eventをグリッドから独立させる方が拡張しやすい。

つまり、

- Legacy import: `LayeredMap.Object/Event` を使う
- New native V2 data: `ObjectSpawn[] / Event[]` を使う

という二段構成でもよい。

---

## 9. 推奨ファイル構成案

例:

```text
dat/stage/5/
  stage0.ini
  terrain0.ary
  visual0.ary
  objects0.csv
  events0.csv
  pipes0.csv

  stage1.ini
  terrain1.ary
  visual1.ary
  objects1.csv
  events1.csv
  pipes1.csv
```

`stage0.ini` では、

- terrain
- visual
- objects
- events
- pipes
- BGM
- background
- scroll
- time

等のファイル・設定を参照。

### 別案

小規模ステージなら `objects/events/pipes` を一つの定義ファイルへまとめてもよい。

重要なのは **概念を分けること** であって、必ずファイル数を増やすことではない。

---

## 10. 旧データ取込方針

旧V1データを直接新形式として使い続けるのではなく、

```text
Version1
 map*.ary
 img*.ary
 Data*.inf
      ↓
 Legacy import / converter
      ↓
Version2 native stage data
 Terrain
 Visual
 ObjectSpawn[]
 Event[]
 Transition[]
 Settings
```

へ寄せるのが最終的には望ましい。

互換ローダーは移行期間・既存資産救済用。

新規ステージまでV1の制約に合わせる必要はない。

---

## 11. 実装残件の推奨順

1. PR #28 Version1 ARY互換ローダーを安定化
2. `Data*.inf` のVersion1 stage definition loader
3. V1 block ID 0..45 → V2 TileCatalog mapping
4. `Action::LoadMapData` のFoundation経路を作る
5. ObjectSpawnの型を新設
6. Legacy enemy `-2..-6` を ObjectSpawnへ変換
7. PlayerをCharacterControllerへ段階移行
8. TileEffect adapter
9. Pipe / stage transitionを本編接続
10. StageProgress → Save / WorldMap
11. Lift / moving object
12. Enemy / Boss / Eventを順に新runtimeへ
13. 旧 `Map/Block` runtimeを撤去
14. 必要なHSP限定機能だけ個別に復活

---

## 12. 現時点の設計判断

### 採用

- TerrainとVisualは分離
- 動的ObjectはTerrainとは別管理
- Version2土管は直接リンク方式
- CanvasMasao由来のterrain semanticsを優先
- C++ V1互換時はV1の実際の挙動を正とする
- HSP旧コードは意味を記録するが勝手に有効化しない
- 旧データと新規V2ネイティブデータは分けて考える

### 非採用 / 復活しない予定

- HSP `mov + coo + inf` をそのままランタイムへ復活
- TerrainとEnemy/Liftを一つのセル値に詰め込む方式
- HSPコード体系をVersion2の正式ID体系にする
- C++ V1のBlockポインタ配列構造を維持

---

## 13. 未決事項

- Object配置ファイルを CSV / INI / JSON のどれにするか
- Event/TriggerをObjectsと同じファイルへまとめるか
- サブマップを `TargetMapId` で扱うか、ステージ定義IDで扱うか
- Visualを常にTerrainと同サイズ必須にするか
- 背景装飾をVisual tileと別レイヤにするか
- Lift経路をObject parameterで持つか、別path定義を持つか
- Legacy importerをランタイムに残すか、最終的にオフラインconverterへ移すか
- HSP限定のスターコイン / checkpoint / sequence 1UPを復活するか

---

## 14. この文書の運用

旧コード調査で新しい事実が分かったら、この文書へ追記する。

特に、

- 「HSPではこうだった」
- 「V1でこう変わった」
- 「V2ではこう再設計した」
- 「意図的に移植しなかった」

を分けて書く。

**古い挙動を見つけたからといって、自動的にVersion2の要件にはしない。**
