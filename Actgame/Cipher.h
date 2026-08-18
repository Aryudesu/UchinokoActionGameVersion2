#pragma once
#include <string>
#include <vector>
void Out(std::string Dat, std::string FileName);
std::string InNoize(std::string Dat);
std::vector<char> XOREnc(std::vector<char> Dat, int Seed);
std::vector<char> Str2Vec(std::string Dat);

std::vector<char> GameEnc(std::string str, std::string FileName,int Key = -1);
std::string GameDec(std::string FileName);