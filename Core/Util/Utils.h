#pragma once
#include <string>
#include <vector>

std::string readShaderFile(const char* fileName);
void        printShaderSource(const char* text);
int         endsWith(const char* s, const char* part);
int         addUnique(std::vector<std::string>& files, const std::string& file);
