#include <iostream>
#include <string>
#include "myparser.h"

using namespace std;

int main(int, char**) {
    MyParser parser;

    string line;
    while(!cin.eof()) {
        cout << "> ";
        getline(cin, line);
        parser.parse(line + '\n');
    }

    return 0;
}
