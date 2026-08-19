#include "function.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#define STRMAX 2048

void Message(const char *Mes) {
	int flag = MessageBox(NULL,TEXT(Mes),TEXT(Mes),MB_YESNO||MB_ICONQUESTION);
}

float AbsF(float x) {
	return (x >= 0) ? x : -x;
}

std::vector<std::string> split(const std::string &str, char sep)
{
	std::vector<std::string> v;
	std::istringstream stream(str);
	std::string field;
	while (std::getline(stream, field, sep))v.push_back(field);
	return v;
}

//次のマルチバイト文字へのポインタを取得
const char* next_c_mb(const char* c) {
	int L = mblen(c, 10);
	return c + L;
}

//マルチバイト文字を一文字取り出す
void ngetc(char* const dst, const char* src) {
	int L = mblen(src, 10);
	memcpy(dst, src, L);
	dst[L] = '\0';
}

//マルチバイト文字を比較する
bool nchr_cmp(const char* c1, const char* c2) {
	int K = mblen(c1, 10);
	int L = mblen(c2, 10);

	if (K != L)
		return false;

	bool issame = (strncmp(c1, c2, K) == 0);
	return issame;
}

std::vector<std::string> split_mb(const char* src, const char* del) {

	char tmp[10];

	std::vector<std::string> result;

	std::string tmps;
	while (*src) {

		//デリミタを飛ばす
		const char* p = src;
		while (nchr_cmp(src, del) == true && *src != '\0')
			src = next_c_mb(src);

		//デリミタに遭遇するまで文字を追加し続ける
		while (nchr_cmp(src, del) != true && *src != '\0') {
			ngetc(tmp, src);//一文字取り出す
			tmps += tmp;
			src = next_c_mb(src);
		}
		if (tmps.size()) {
			result.push_back(tmps);
		}
		tmps.clear();
	}

	return result;
}

std::vector<int> SplitNum(const std::string &str, char sep) {
	std::vector<std::string> tmp = split(str,sep);
	std::vector<int> Data;
	for (int i = 0; i < tmp.size(); i++)Data.push_back(std::stoi(tmp[i]));
	return Data;
}

//String型でCSV読み込み
std::vector<std::vector<std::string>> LoadStrArray(std::string FileName) {
	std::vector<std::vector<std::string>> Data;
	std::string str;
	int fp = FileRead_open(FileName.c_str());
	if (fp < 0) {
		const std::string ErrorMessage = "LoadStrArray() Error\n" + FileName;
		Message(ErrorMessage.c_str());
		exit(0);
	}
	char buf[STRMAX];
	while (FileRead_eof(fp) == 0) {
		FileRead_gets(buf, STRMAX, fp);
		std::vector<std::string> inner;
		std::string str = buf;
		inner = split_mb(str.c_str(), ",");
		Data.push_back(inner);
	}
	FileRead_close(fp);
	return Data;
}

//int型でCSV読み込み
std::vector<std::vector <int>> LoadArray(std::string FileName) {
	std::vector<std::vector<int>> Data;
	std::vector<std::vector<std::string>> tmp = LoadStrArray(FileName);
	for (int i = 0; i < tmp.size(); i++) {
		std::vector<int> NumArr;
		for (int j = 0; j < tmp[i].size(); j++) {
			NumArr.push_back(std::stoi(tmp[i][j]));
		}
		Data.push_back(NumArr);
	}
	return Data;
}

double sign(double x) {
	return x / abs(x);
}

int sign(int x) {
	return x / abs(x);
}

void SetBright(int Num) { SetDrawBright(Num, Num, Num); }

void SetTitleStr(std::string Mes) { SetWindowText(Mes.c_str()); }

bool check_int(std::string str) {
	if (std::all_of(str.cbegin(), str.cend(), isdigit))return true;
	return false;
}

bool check_arr(std::string str) {
	if (str.find(",") != std::string::npos)return true;
	return false;
}

INIDat* LoadDataList(std::string FileName) {
	return new INIDat(FileName);
}

INIDat* LoadStageData(int StageNum) {
	std::string Path = "dat/stage/" + std::to_string(StageNum) + "/";
	std::string Dat = Path + "Data0.inf";
	return LoadDataList(Dat);
}

INIDat* LoadStageData(int StageNum, int StageDetail) {
	std::string Path = "dat/stage/" + std::to_string(StageNum) + "/";
	std::string Dat = Path + "Data" + std::to_string(StageDetail) + ".inf";
	return LoadDataList(Dat);
}


std::string trim(const std::string& str, const char* trimCharacterList) {
	std::string result;
	std::string::size_type left = str.find_first_not_of(trimCharacterList);
	if (left != std::string::npos) {
		std::string::size_type right = str.find_last_not_of(trimCharacterList);
		result = str.substr(left, right - left + 1);
	}
	return result;
}
