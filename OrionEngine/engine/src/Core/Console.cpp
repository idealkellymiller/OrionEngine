#include "Core/Console.h"


void Console::Log(std::string message) {
	std::cout << message << "\n" << std::endl;
}

void Console::LogWarning(std::string message) {
	std::cout << message << "\n" << std::endl;
}

void Console::LogError(std::string message) {
	std::cout << message << "\n" << std::endl;
}