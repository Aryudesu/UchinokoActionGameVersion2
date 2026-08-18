#pragma once
#include <vector>
#include <string>
#include "Scene.h"

class MusicRoom : public Scene {
	std::vector<std::string> ListTitle;
	std::vector<std::string> Title;
	std::vector<std::string> Auther;
	std::vector<int> LoopPoint;
	std::vector<std::string> FileName;
	int Select = 0;
	int Row = 0, RowMax = 0;
	bool Play = false;
	void PlayMusic(int n);
	int StrX,PlayNum;
	void FFT();
	int SoundHandle;
	int SoftSoundHandle;
public:
	MusicRoom();
	void InputDat();
	void update();
	void draw();
};