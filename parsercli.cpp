#include <iostream>
#include <string>
#include "myparser.h"

using namespace std;

int main(int, char**) {
	string line;
	while(!cin.eof()) {
		cout << "> ";
		getline(cin, line);
		parse(line + '\n');
	}

	return 0;
}
