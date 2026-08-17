//ゲーム本体クラス
#pragma once
class System {
private:
public:
	bool initialize();
	void finalize();
	System() = default;
	~System() = default;

	void MainLoop();	//ループ関数（ここで裏画面処理を行っている）
};