#pragma once
#include <string>

std::string Base64Enc(std::string mes);
std::string Base64Dec(std::string mes);
//Base64ベースシーザー暗号用
unsigned char ShiftB64(unsigned char c, int Key);
//Base64ベース暗号化
std::string CaesarB64(std::string mes, int Key = 3);