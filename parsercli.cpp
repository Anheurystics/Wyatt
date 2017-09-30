#include <iostream>
#include <string>
#include "myparser.h"

int main(int, char**) {
	std::string line;
	while(!std::cin.eof()) {
		std::cout << "> ";
		std::getline(std::cin, line);
		parse(line + '\n');
	}

	return 0;
}
