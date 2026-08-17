#pragma once
#include "Singleton.h"
#include "Dxlib.h"
#include <vector>
#include <string>

//-----------BGM用-----------
#define BGM1  0


//-----------SE用-----------
#define THREAD 0
#define COIN   1
#define JUMP1  2
#define JUMP2  3
#define JUMP3  4
#define DAMAG  5
#define LADDER 6
#define EXPOSE 7
#define HIT	   8
#define PIPE   9
#define DEAD  10
#define BEAT  11
#define EXTEND 12
#define DECIDE 13
#define BROKEN 14
#define HITBLK 15
#define DAPBLK 16
#define SWIT   17
#define WATER  18

class SoundManager : public Singleton<SoundManager> {
	std::vector<int> SE;
	std::vector<int> BGM;
	int Volume;
	int SEVolume;
	int VolumeCount;
	int SEVolumeCount;
	int DrawCount;
	int SoftSoundHandle;
public:
	SoundManager();
	void SetSE(int Num, std::string FileName);
	void PlaySE(int Num);
	void SetBGM(int Num, int LoopPoint, std::string FileName);
	int  SetSSBGM(int Num, int LoopPoint, std::string FileName);
	void PlayBGM(int Num);
	void PlaySSBGM(int Num);
	void StopBGM(int Num);
	void StopSSBGM(int Num);
	void DeleteBGM(int Num);
	void DeleteSSBGM(int Num);
	void ChangeVolume(int V);
	void ChangeBGMVolume(int V);
	void ConfBGMVolume();
	void ConfVolume();
	void Draw();
	int  GetSoftSoundHandle();
	int  GetSSBGMHandle();
	int  GetVolume();
};