#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <iostream>
#include <cstring>
#include <QPlainTextEdit>
#include <QTextCursor>

#include "nodes.h"

using namespace std;

struct LogInfo {
    string label = "";
    unsigned int first_line = 0, last_line = 0;
    unsigned int first_column = 0, last_column = 0;
};

class LogWindow : public QPlainTextEdit
{
    public:
        explicit LogWindow(QWidget *parent);

        void log(string);
        void log(LogInfo info, string);
        void log(shared_ptr<Node>, string, string);
        void clear();
};

#endif // LOGWINDOW_H
