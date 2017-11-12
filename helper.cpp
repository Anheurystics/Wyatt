#include "helper.h"

string str_from_file(string filename) {
    string text = "";
    ifstream file;
    file.open(filename, ios::in);
    if(file.is_open()) {
        string line;
        while(getline(file, line)) {
            text += line + '\n';
        }
        file.close();
    } else {
       cout << "File not found: " << filename << endl;
    }

    return text;
}

