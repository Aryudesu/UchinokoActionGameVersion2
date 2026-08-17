#include "Base64.h"
#include <string>
#include <vector>

//Base64 Encode
std::string Base64Enc(std::string mes) {
	std::string result;
	std::string EList = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::vector<bool> BList;
	for (int i = 0; i < mes.size(); i++) {
		unsigned char tmp = mes[i];
		for (int j = 0; j < 8; j++)BList.push_back((tmp >> (7 - j)) & (unsigned char)1);
	}
	if (BList.size() % 6 != 0) {
		for (int i = 0; i < (BList.size() % 6); i++)BList.push_back(false);
	}
	int tmp;
	for (int i = 0; i <= BList.size(); i++) {
		if (i % 6 == 0) {
			if (i != 0)result += std::string() + EList[tmp];
			if (i == BList.size())break;
			tmp = 0;
		}
		else {
			tmp <<= 1;
		}
		if (BList[i])tmp += 1;
	}
	if (result.size() % 4 != 0) {
		for (int i = 0; i < (result.size() % 4); i++)result += "=";
	}
	return result;
}

//Base64 Decode
std::string Base64Dec(std::string mes) {
	unsigned char a = (unsigned char)'a', z = (unsigned char)'z', BA = (unsigned char)'A', BZ = (unsigned char)'Z', Zero = (unsigned char)'0', Nine = (unsigned char)'9';
	std::vector<bool>BList;
	std::string result;
	for (int i = 0; i < mes.size(); i++) {
		unsigned char c = (unsigned char)mes[i];
		if (c == '=')break;
		char tmp = 0;
		if (c >= BA && c <= BZ)tmp = (c - BA);
		else if (c >= a && c <= z)tmp = (c - a) + 26;
		else if (c >= Zero && c <= Nine)tmp = (c - Zero) + 52;
		else if (c == '+')tmp = 62;
		else if (c == '/')tmp = 63;
		for (int j = 0; j < 6; j++)BList.push_back((tmp >> (5 - j)) & (char)1);
	}
	for (int i = 0; i < BList.size() / 8; i++) {
		char c = 0;
		for (int j = 0; j < 8; j++) {
			c <<= 1;
			if (BList[i * 8 + j])c |= (char)1;
		}
		result += std::string() + c;
	}
	return result;
}

unsigned char ShiftB64(unsigned char c, int Key) {
	std::string EList = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	for (int i = 0; i < EList.size(); i++) {
		if (EList[i] == c) {
			int tmp = (int)i + Key;
			while (tmp < 0)tmp += EList.size();
			return EList[tmp % EList.size()];
		}
	}
	return '=';
}
std::string CaesarB64(std::string mes, int Key) {
	std::string result = mes;
	for (int i = 0; i < mes.size(); i++)result[i] = ShiftB64((unsigned char)mes[i], Key);
	return result;
}