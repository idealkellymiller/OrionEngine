#pragma once
#include <string>
#include <iostream>

class Console {
public:
	static void Log(std::string message);
	static void LogWarning(std::string message);
	static void LogError(std::string message);
};