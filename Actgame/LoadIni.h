#pragma once
#include <string>
#include <vector>

class INIDat {
	std::vector<std::string> Section;
	std::vector<std::vector<std::vector<std::string>>> SDList;

public:
	INIDat();
	INIDat(std::string FileName);
	~INIDat();
	void DataInput(std::string FileName);
	void DataDelete();
	int GetSecNum(std::string Sec);
	bool CheckSec(std::string Sec);
	bool CheckElem(std::string Sec,std::string Elem);
	std::vector<std::string> GetData(std::string Sec, std::string Elem);

	void ShowAllData();
};