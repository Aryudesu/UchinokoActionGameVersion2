# C++ Version1仕様

C++ Version1の実コード・正規ステージデータを基準にした仕様メモです。

HSP版から引き継いだ値がデータ上に残っていても、**Version1実コードがどう扱っているかを優先**します。

---

## 1. ステージデータ構成

正規ステージは概ね次の組で構成されます。

```text
dat/stage/<Stage>/
  Data0.inf
  map0.ary
  img0.ary

  Data1.inf
  map1.ary
  img1.ary
  ...
```

### map*.ary

ブロック・Player開始位置・一部Enemy配置を保持。

### img*.ary

各セルの見た目用画像番号。

`Map::SetMapChip` で、`map` から生成済みのBlockへ画像番号を設定します。

### Data*.inf

サブマップごとの設定。

主な項目:

- `GameMode`
- `Time`
- `ScrollMode`
- `StageMoving`
- `AppearData`
- `ImgDat`
- `BGMData`
- `SEData`

---

## 2. Map::LoadMap

実行時のmap値解釈:

| 値 | Version1動作 |
|---:|---|
| `0..50` | `BlockFactory` へ渡す |
| `-1` | Player初期位置 |
| `-2` | EnemyKind 1 |
| `-3` | EnemyKind 2 |
| `-4` | EnemyKind 3 |
| `-5` | EnemyKind 4 |
| `-6` | EnemyKind 5 |
| その他 | Empty |

EnemyFactoryと `enemy.bmp` を合わせると、Enemy markerは次のクラスへ対応します。

| map marker | EnemyKind | Class | enemy.bmp row |
|---:|---:|---|---:|
| `-2` | 1 | `WalkingEnemy1` | 0 |
| `-3` | 2 | `WalkingEnemy2` | 1 |
| `-4` | 3 | `CarrotMan` | 2 |
| `-5` | 4 | `BallSlime` | 3 |
| `-6` | 5 | `BallSlime2` | 4 |

`enemy.bmp` はVersion1で32×32、12列×7行としてロードされ、各クラスは `BaseImg = 12 * row` を持ちます。

ただし現行 `BlockFactory` で意味を持つのは `0..45` で、`46..50` は結果的にEmptyになります。

### HSP由来コード

正規V1データにも、

- `110`
- `301`
- `321..325`
- `-79`

等が残っています。

HSP版では意味を持っていましたが、C++ Version1の `Map::LoadMap` では上記特例に該当しないためEmpty扱いです。

---

## 3. BlockFactory ID

現行実コードの `BlockFactory.cpp` を基準にします。

| ID | Class / 意味 |
|---:|---|
| 0 | Empty |
| 1 | Steals / 通常Solid |
| 2 | Bricks |
| 3 | Coin |
| 4 | CoinBlock |
| 5 | Coin10Block |
| 6 | HealingBlock |
| 7 | OneUpBlock |
| 8 | LadderMakerBlock |
| 9 | BrownBlock |
| 10 | Ladder |
| 11 | Cloud |
| 12 | Through |
| 13 | PipeUL |
| 14 | PipeUR |
| 15 | PipeDL |
| 16 | PipeDR |
| 17 | PipeLU |
| 18 | PipeLD |
| 19 | PipeRU |
| 20 | PipeRD |
| 21 | ONOFF |
| 22 | WaterBlock |
| 23 | HealingCoin |
| 24 | StealCoinBlock |
| 25 | OneUPCoin |
| 26 | StealHealingBlock |
| 27 | StealOneUpBlock |
| 28 | StealLadderMakerBlock |
| 29 | ZeroCoinBlock |
| 30 | BeatItem / Goal |
| 31 | BeatItem2 |
| 32 | UpGravBlock |
| 33 | DownGravBlock |
| 34 | DisAppBlock1 |
| 35 | DisAppBlock2 |
| 36 | O50CoinBlock |
| 37 | U50CoinBlock |
| 38 | ONOFFDABlock1 |
| 39 | ONOFFDABlock2 |
| 40..45 | Steals + Kill = ID - 39 |

古い `Memo.txt` や `仕様.txt` には別世代の番号体系も含まれるため、現行コードと異なる場合は `BlockFactory.cpp` を優先します。

---

## 4. サブマップ・土管移動

Version1ではHSP版より簡単化されています。

### StageMoving

`Data0.inf` 例:

```ini
StageMoving = 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0
```

PlayerのX位置を10タイル単位で区切り、移動先 `StageDetailNum` を決めます。

### AppearData

```ini
>AppearData
1 = 1,97.5,22
```

キーは移動元サブマップ番号。

値は、

```text
出現演出, X, Y
```

として使われます。

### Action::Move

移動時は、

1. 現在のObject/Image/Sound等を破棄
2. `StageDetailNum` を変更
3. `Data<detail>.inf` を再ロード
4. `map<detail>.ary / img<detail>.ary` をロード
5. `AppearData` に従いPlayerを再配置

という流れです。

---

## 5. Player / Map runtime

Version1の本編runtimeは、

```text
Action
 ├─ Map
 │   └─ vector<vector<Block*>>
 ├─ PlayerManager
 ├─ ObjectManager
 ├─ ImageManager
 ├─ SoundManager
 └─ GameData
```

という構造です。

Playerの移動・衝突処理は `Map*` を直接参照し、

- `GetNum`
- `GetKill`
- `Hited`
- `Touched`
- `SetBlock`

等を通じてBlockと相互作用します。

---

## 6. GameData / Save

主な状態:

- 残機
- Score
- Time
- WorldMap位置
- `BeatLevel[1500]`
- Hidden block用状態
- ON/OFF状態

Save時には、

- 残機
- HP
- Score
- Coin
- WorldMap X/Y
- BeatLevel

を保存します。

Version1では `BeatLevel` は実質的に「クリア済み」の整数配列として使われ、通常ゴールと裏ゴールの区別は保持されません。

---

## 7. WorldMap

WorldMapのマップ値と `BeatLevel` を使って通路・ステージ状態を開放します。

代表的な扱い:

- 正のステージ番号: ステージポイント
- 負の番号: 対応ステージクリアで通行可能にするセル
- `999`: 初期位置
- `<= -500`: 自動会話イベント等

WorldMap上の位置もSaveされます。

---

## 8. Version1の課題

Version2移植時に問題になる主な点:

- `Block*` の2次元配列へ挙動を埋め込みすぎている
- PlayerとMapが強く結合
- 敵スポーンがmap値へ混在
- TerrainとObjectを同じセルへ自由に重ねにくい
- サブマップ移動がX位置依存
- Goal状態が通常/裏で分離されていない
- Save / WorldMapが `BeatLevel` 整数配列へ依存
- HSP由来の無効コードが正規データ内に残っている

Version2ではこれらを段階的に分離します。
