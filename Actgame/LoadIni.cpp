#include "LoadIni.h"
#include "function.h"

void INIDat::DataInput(std::string FileName) {
	int fp = FileRead_open(FileName.c_str());

	std::vector<std::vector<std::string>> tmpList;
	bool SectChange = false;
	bool FirstSect = false;

	char buf[256];

	while (FileRead_eof(fp) == 0) {
		FileRead_gets(buf, 256, fp);	//1行読み込み
		std::string str = buf;			//読み込んだやつstring型に格納

		if (str[0] == '#')continue;
		if (str[0] == '>') {
			str.erase(str.begin());
			Section.push_back(trim(str));
			SectChange = true;
			if (FirstSect) {
				SDList.push_back(tmpList);
				int N = tmpList.size();
				for (int i = 0; i < N; i++) {
					tmpList[i].clear();
				}
				tmpList.clear();
			}
			FirstSect = true;
			continue;
		}
		std::vector<std::string> tmp = split(str, '=');	// = で切り取る
		std::vector<std::string> tmp2;					//一時保存用
		if (tmp.size() <= 1)continue;
		tmp2.push_back(trim(tmp[0]));							// = より左側を格納
		if (check_arr(tmp[1])) {						//右側が配列データのとき
			std::vector<std::string> tmp3 = split(tmp[1], ',');	//配列データをバラす
			for (int i = 0; i < tmp3.size(); i++)tmp2.push_back(trim(tmp3[i]));	//バラした配列データを格納
		} else {
			tmp2.push_back(trim(tmp[1]));		//配列じゃなかったらそのまま格納
		}
		tmpList.push_back(tmp2);			//結果に格納
	}
	SDList.push_back(tmpList);
	FileRead_close(fp);
}

void INIDat::DataDelete() {
	for (int i = 0; i < SDList.size(); i++) {
		for (int j = 0; j < SDList[i].size(); j++) {
			SDList[i][j].clear();
			SDList[i][j].shrink_to_fit();
		}
		SDList[i].clear();
		SDList[i].shrink_to_fit();
	}
	SDList.clear();
	SDList.shrink_to_fit();

	Section.clear();
	Section.shrink_to_fit();
}

int INIDat::GetSecNum(std::string Sec) {
	if (!CheckSec(Sec)) {
		Message("Null Pointer Exception");
		exit(0);
	}
	for (int i = 0; i < Section.size(); i++) {
		if (Section[i] == Sec)return i;
	}
	return -1;
}

bool INIDat::CheckSec(std::string Sec) {
	for (int i = 0; i < Section.size(); i++) {
		if (Section[i] == Sec)return true;
	}
	return false;
}

bool INIDat::CheckElem(std::string Sec, std::string Elem) {
	if (!CheckSec(Sec))return false;
	int num = GetSecNum(Sec);
	for (int i = 0; i < SDList[num].size(); i++) {
		//Message((SDList[num][i][0] + " : " + Elem).c_str());
		if (SDList[num][i][0] == Elem)return true;
	}
	return false;
}

std::vector<std::string> INIDat::GetData(std::string Sec, std::string Elem) {
	if (!CheckSec(Sec) || !CheckElem(Sec, Elem)) {
		Message("Null Pointer Exception");
		exit(0);
	}
	int num = GetSecNum(Sec);
	for (int i = 0; i < SDList[num].size(); i++) {
		if (SDList[num][i][0] == Elem) {
			std::vector<std::string> result;
			for (int j = 1; j < SDList[num][i].size(); j++) {
				result.push_back(SDList[num][i][j]);
			}
			return result;
		}
	}
	return {};
}

INIDat::INIDat() {}

INIDat::INIDat(std::string FileName) { DataInput(FileName); }

INIDat::~INIDat() { DataDelete(); }

void INIDat::ShowAllData() {
	for (int i = 0; i < Section.size(); i++) {
		Message(Section[i].c_str());
		for (int j = 0; j < SDList[i].size(); j++) {
			for(int k=0;k<SDList[i][j].size();k++)Message(SDList[i][j][k].c_str());
		}
	}
}