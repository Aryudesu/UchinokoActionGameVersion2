#include "DxLib.h"
#include "Conf.h"
#include "System.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	System GameMain;
	if (GameMain.initialize()) {
		GameMain.MainLoop();
	}
	GameMain.finalize();
	return 0;
}
