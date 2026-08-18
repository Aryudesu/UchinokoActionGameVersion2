#include "Dxlib.h"
#include "Singleton.h"
#include <vector>
#pragma once

constexpr int KEY_NUM = 256;

//キー操作クラス

class InputKey :public Singleton<InputKey>{
	char tmp[KEY_NUM];
	unsigned int Key[KEY_NUM];
public:
	friend class Singleton<InputKey>;

	//キー状態更新
	int UpdateKey();

	//指定されたキー状態を得る
	int ReturnKey(int KeyNum);
};


//キー状態更新
int KeyUpdate();

//キー状態取得
int ReturnKey(int KeyNum);