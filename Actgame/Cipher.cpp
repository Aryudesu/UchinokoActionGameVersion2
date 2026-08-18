#include "Cipher.h"
#include "Base64.h"
#include <iostream>
#include <fstream>

void Out(std::vector<char> Dat, std::string FileName) {
	std::ofstream fout;
	fout.open(FileName, std::ios::out | std::ios::binary | std::ios::trunc);
	int Size = Dat.size();
	//printf("\nOutputSize : %d\n\n", Size);
	fout.write(&Dat[0], sizeof(char) * Dat.size());
	fout.close();
}

std::string InNoize(std::string Dat) {
	char t[2]; t[1] = '\0';
	std::string EList = "!?<>#$%&'()*~|;:@`-^\\\".,_[]";	//いらない文字
	std::string Result = Dat;							//いらない文字挿入
	int Num = rand() % (Dat.size() / 2);						//何文字挿入するか
	for (int i = 0; i < Num; i++) {
		t[0] = EList[rand() % EList.size()];				//不要文字選ぶ
		Result = Result.insert(rand() % (Result.size() - 1), t);	//挿入
	}
	t[0] = EList[rand() % EList.size()];
	std::string str = t;
	Result = str + Result;		//シーザ暗号鍵が先頭に来て不要文字が来るまで
	return Result;
}

std::vector<char> XOREnc(std::vector<char> Dat, int Seed) {
	srand(Seed);
	for (int i = 0; i < Dat.size(); i++) {
		char rnd = (unsigned char)(rand() % 256);
		Dat[i] = (char)(Dat[i] ^ rnd);
	}
	return Dat;
}

std::vector<char> Str2Vec(std::string Dat) {
	std::vector<char> Result(Dat.size());
	for (int i = 0; i < Dat.size(); i++)Result[i] = Dat[i];
	return Result;
}

std::vector<char> GameEnc(std::string str, std::string FileName,int Key) {
	int Key1 = rand() % 500;			//データ書き換え管理用
	int Key2 = rand() % 100;			//シーザ暗号用
	char t[2]; t[1] = '\0';
	std::string Str;
	if (Key >= 0) {
		Str = std::to_string(Key % Key1) + "," + std::to_string(Key1) + "," + str;	//データ書き換え管理用
	}
	std::string B64 = Base64Enc(Str);			//64エンコード
	std::string B64C = CaesarB64(B64, Key2);	//シーザ暗号化
	std::string NB64C = InNoize(B64C);			//ノイズデータ混入
	NB64C = std::to_string(Key2) + NB64C;		//シーザ暗号鍵が先頭に来て不要文字が来るまで
	int Seed = rand() % 5000;					//乱数シード
	std::vector<char> Vec = XOREnc(Str2Vec(NB64C), Seed);
	Vec.insert(Vec.begin(), { (char)(Seed / 256), (char)(Seed % 256) });
	int Size = Vec.size() + 2;
	Vec.insert(Vec.begin(), { (char)(Size / 256), (char)(Size % 256) });
	Out(Vec,FileName);
	return Vec;
}

std::string GameDec(std::string FileName) {
	std::ifstream fin(FileName, std::ios::binary);
	std::string Dat;
	char t[2]; t[1] = '\0';
	int Size = 0;
	char c;
	for (int i = 0; i < 2; i++) {
		fin.read((char*)&c, sizeof(char));
		Size = Size * 256 + (unsigned char)c;
	}
	fin.seekg(0);
	char* data = new char[Size + 1];
	for (int i = 0; i < Size; i++) {
		fin.read((char*)&c, sizeof(char));
		data[i] = c;
	}
	data[Size] = '\0';
	fin.close();
	for (int i = 0; i < Size - 2; i++)data[i] = data[i + 2];
	int Seed = (unsigned char)data[0] * 256 + (unsigned char)data[1];
	for (int i = 0; i < Size - 4; i++)data[i] = data[i + 2];
	srand(Seed);
	for (int i = 0; i < Size - 4; i++) {
		char rnd = (unsigned char)(rand() % 256);
		data[i] = (char)(data[i] ^ rnd);
	}
	int Key = 0;
	std::string tmp;
	bool f = false;
	for (int i = 0; i < Size - 4; i++) {
		unsigned char ch = data[i];
		if (f) {
			if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '+' || ch == '/' || ch == '=') {
				t[0] = ch;
				tmp += t;
			}
		}
		else {
			if (ch >= '0' && ch <= '9')Key = Key * 10 + ((unsigned char)ch - (unsigned char)'0');
			else f = true;
		}
	}
	std::string B64 = CaesarB64(tmp, -Key);
	std::string Result = Base64Dec(B64);
	return Result;
}