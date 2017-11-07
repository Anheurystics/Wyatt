#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <iostream>
#include <QPlainTextEdit>
#include <QTextCursor>

class LogWindow : public QPlainTextEdit
{
    public:
        explicit LogWindow(QWidget *parent);

        void log(std::string);
        void clear();
};

#endif // LOGWINDOW_H
