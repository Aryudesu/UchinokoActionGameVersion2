#pragma once

//ウィンドウサイズとかの変数設定

//マップチップサイズ
#define MAPSIZEX 32
#define MAPSIZEY 32

//画像幅からの空き
#define GAPX 12
#define GAPY 0

//オブジェクトチップサイズ
#define OBJSIZEX 32
#define OBJSIZEY 32



//画面サイズ（マップチップサイズ計算）
#define SCREENX 24//(3*16/2)
#define SCREENY 14//(3*10/2)


//画面サイズ（ピクセル）
#define WINDOWX (SCREENX*MAPSIZEX)
#define WINDOWY (SCREENY*MAPSIZEY)

//透過色
#define TRANSR 111
#define TRANSG 49
#define TRANSB 152