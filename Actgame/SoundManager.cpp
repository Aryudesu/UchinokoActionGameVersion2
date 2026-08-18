#include "SoundManager.h"
#include "Dxlib.h"
#include "InputKey.h"
#include <vector>
#include <string>

SoundManager::SoundManager() {
	SE.resize(30);
	BGM.resize(3);
	DrawCount = 0;
	Volume = 60;
	VolumeCount = 0;
	SEVolume = 50;
	SEVolumeCount = 0;
	ChangeBGMVolume(Volume);			//音量
	ChangeVolume(SEVolume);			    //SE音量
	SoftSoundHandle = -1;
}

void  SoundManager::SetSE(int Num, std::string FileName) {
	SE[Num] = LoadSoundMem(FileName.c_str());
}

void  SoundManager::PlaySE(int Num) {
	SoundManager::GetInstance().ChangeVolume(SEVolume);
	PlaySoundMem(SE[Num], DX_PLAYTYPE_BACK);
}

void  SoundManager::SetBGM(int Num, int LoopPoint, std::string FileName) {
	BGM[Num] = LoadSoundMem(FileName.c_str());
	SetLoopPosSoundMem(LoopPoint, BGM[Num]);
}

int  SoundManager::SetSSBGM(int Num, int LoopPoint, std::string FileName) {
	SoftSoundHandle = LoadSoftSound(FileName.c_str());
	BGM[Num] = LoadSoundMemFromSoftSound(SoftSoundHandle);
	SetLoopPosSoundMem(LoopPoint, BGM[Num]);
	return BGM[Num];
}

void  SoundManager::PlayBGM(int Num) {
	SoundManager::GetInstance().ChangeBGMVolume(Volume);
	PlaySoundMem(BGM[Num], DX_PLAYTYPE_LOOP);
}

void SoundManager::PlaySSBGM(int Num) {
	SoundManager::GetInstance().ChangeBGMVolume(Volume);
	PlaySoundMem(BGM[Num], DX_PLAYTYPE_LOOP);
}

void  SoundManager::StopBGM(int Num) {
	StopSoundMem(BGM[Num]);
}

void  SoundManager::StopSSBGM(int Num) {
	int Tmp = LoadSoundMemFromSoftSound(SoftSoundHandle);
	StopSoundMem(Tmp);
}

void SoundManager::DeleteBGM(int Num) {
	DeleteSoundMem(BGM[Num]);
}

void SoundManager::DeleteSSBGM(int Num) {
	SoundManager::GetInstance().ChangeBGMVolume(Volume);
	int Tmp = LoadSoundMemFromSoftSound(SoftSoundHandle);
	DeleteSoundMem(Tmp);
}

void SoundManager::ChangeVolume(int V) {
	for (int i = 0; i < SE.size(); i++) {
		ChangeVolumeSoundMem(255 * V / 100, SE[i]);
	}
}

void SoundManager::ChangeBGMVolume(int V) {
	ChangeVolumeSoundMem(255 * V / 100, BGM[BGM1]);
}

void SoundManager::ConfBGMVolume() {
	if (ReturnKey(KEY_INPUT_LSHIFT) > 0 || ReturnKey(KEY_INPUT_RSHIFT) > 0) {
		if (ReturnKey(KEY_INPUT_SEMICOLON) > 0) {
			VolumeCount++;
			int tmp = 10;
			if (VolumeCount > 50)tmp = 5;
			if (VolumeCount % tmp == 1) {
				Volume++;
				DrawCount = 1;
				if (Volume > 100)Volume = 100;
				SoundManager::GetInstance().ChangeBGMVolume(Volume);
			}
		}
		if (ReturnKey(KEY_INPUT_MINUS) > 0) {
			VolumeCount++;
			int tmp = 10;
			if (VolumeCount > 50)tmp = 5;
			if (VolumeCount % tmp == 1) {
				Volume--;
				DrawCount = 1;
				if (Volume < 0)Volume = 0;
				SoundManager::GetInstance().ChangeBGMVolume(Volume);
			}
		}
		if (ReturnKey(KEY_INPUT_MINUS) == 0 && ReturnKey(KEY_INPUT_SEMICOLON) == 0)VolumeCount = 0;
	} else {
		VolumeCount = 0;
	}
}

void SoundManager::ConfVolume() {
	if (ReturnKey(KEY_INPUT_LCONTROL) > 0 || ReturnKey(KEY_INPUT_RCONTROL) > 0) {
		if (ReturnKey(KEY_INPUT_SEMICOLON) > 0) {
			SEVolumeCount++;
			int tmp = 10;
			if (SEVolumeCount > 50)tmp = 5;
			if (SEVolumeCount % tmp == 1) {
				SEVolume++;
				DrawCount = 1;
				if (SEVolume > 100)SEVolume = 100;
				SoundManager::GetInstance().ChangeVolume(SEVolume);
			}
		}
		if (ReturnKey(KEY_INPUT_MINUS) > 0) {
			SEVolumeCount++;
			int tmp = 10;
			if (SEVolumeCount > 50)tmp = 5;
			if (SEVolumeCount % tmp == 1) {
				SEVolume--;
				DrawCount = 1;
				if (SEVolume < 0)SEVolume = 0;
				SoundManager::GetInstance().ChangeVolume(SEVolume);
			}
		}
		if (ReturnKey(KEY_INPUT_MINUS) == 0 && ReturnKey(KEY_INPUT_SEMICOLON) == 0)SEVolumeCount = 0;
	}
	else {
		SEVolumeCount = 0;
	}
}

void SoundManager::Draw() {
	if (DrawCount > 0)DrawCount++;
	if (DrawCount > 100)DrawCount = 0;
	if (DrawCount > 0) {
		DrawString(4,  4, ("BGM : " + std::to_string(Volume)).c_str(), GetColor(255, 0, 0));
		DrawString(4, 20, ("SE  : " + std::to_string(SEVolume)).c_str(), GetColor(255, 0, 0));
	}
}

int SoundManager::GetSoftSoundHandle() {
	return SoftSoundHandle;
}

int SoundManager::GetSSBGMHandle() {
	return LoadSoundMemFromSoftSound(SoftSoundHandle);
}

int SoundManager::GetVolume() {
	return Volume;
}