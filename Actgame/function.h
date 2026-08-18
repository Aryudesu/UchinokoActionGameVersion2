#pragma once
#include <vector>
#include <string>
#include "Dxlib.h"
#include "LoadIni.h"

//デバッグ用.メッセージを出力
void Message(const char *Mes);

//絶対値
float AbsF(float x);

//String型を指定された文字で区切ったものをstring配列で返す
std::vector<std::string> split(const std::string &str, char sep);

//String型で指定された文字で区切ったものをint配列で返す
std::vector<int> SplitNum(const std::string &str, char sep);

//String型でCSV読み込み
std::vector<std::vector<std::string>> LoadStrArray(std::string FileName);

//int型でCSV読み込み
std::vector<std::vector <int>> LoadArray(std::string FileName);

//引数が＋なら1、ーなら-1を返す
double sign(double x);

//引数が＋なら1、ーなら-1を返す
int sign(int x);

void SetBright(int Num);

INIDat* LoadStageData(int StageNum);

INIDat* LoadStageData(int StageNum, int StageDetail);

void SetTitleStr(std::string Mes);

bool check_int(std::string str);
bool check_arr(std::string str);

INIDat* LoadDataList(std::string FileName);

std::string trim(const std::string& str,const char* trimCharacterList = " \n\t\0");