#include "InputKey.h"


int InputKey::UpdateKey() {
	GetHitKeyStateAll(tmp);
	for (int i = 0; i < KEY_NUM; i++) {
		if (tmp[i] != 0) {
			Key[i]++;
		}
		else {
			Key[i] = 0;
		}
	}
	return 0;
}

//指定されたキー状態を得る
int InputKey::ReturnKey(int KeyNum) {
	return Key[KeyNum];
}

int KeyUpdate() {
	InputKey& Key = InputKey::GetInstance();
	Key.UpdateKey();
	return 0;
}

//キー状態取得
int ReturnKey(int KeyNum) {
	InputKey& Key = InputKey::GetInstance();
	return Key.ReturnKey(KeyNum);
}