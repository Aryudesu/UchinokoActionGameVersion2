#include "System.h"
#include "MainLoop.h"
#include "DxLib.h"
#include "Conf.h"
#include "time.h"
#include "function.h"
#include "SoundManager.h"
#include "InputKey.h"

class Fps {
	int mStartTime;
	int mCount;
	float mFps;
	static const int N = 60;
	static const int FPS = 60;
public:
	Fps() {
		mStartTime = 0;
		mCount = 0;
		mFps = 0;
	}
	bool Update() {
		if (mCount == 0) {
			mStartTime = GetNowCount();
		}
		if (mCount == N) {
			int t = GetNowCount();
			mFps = 1000.f / ((t - mStartTime) / (float)N);
			mCount = 0;
			mStartTime = t;
		}
		mCount++;
		return true;
	}
	void Wait() {
		int tookTime = GetNowCount() - mStartTime;
		int waitTime = mCount * 1000 / FPS - tookTime;
		if (waitTime > 0) {
			Sleep(waitTime);
		}
	}
};

bool System::initialize() {
	SetAlwaysRunFlag(TRUE);										//ノンアクティブでも実行
	SetWindowSizeChangeEnableFlag(TRUE);						//ウィンドウサイズ変更
	SetOutApplicationLogValidFlag(FALSE);						//ログ出力
	ChangeWindowMode(TRUE);										//ウインドウモード
	SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_DESKTOP);	//フルスクリーンでも縦横比保持
	SetWindowText("自作アクションゲー");							//ウィンドウタイトル
	SetGraphMode(WINDOWX, WINDOWY, 32);							//ウインドウサイズとか
	SetEnableXAudioFlag(FALSE);
	if (DxLib_Init() == -1)return false;								//初期化
	SetDrawScreen(DX_SCREEN_BACK);								//裏画面処理
	SRand((unsigned)time(NULL));
	return true;
}

void System::finalize() {
	DxLib_End();												// DXライブラリ終了処理
}

void System::MainLoop() {
	GameBody MainGame;
	Fps fps;
	while (ProcessMessage() == 0) {
		fps.Update();
		ClearDrawScreen();
		if (!MainGame.loop())break;
		if (ScreenFlip() == -1) {
			Message("Error");
			exit(0);
		}
		fps.Wait();
	}
}