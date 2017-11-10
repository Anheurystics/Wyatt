#include "logwindow.h"

LogWindow::LogWindow(QWidget *parent = 0): QPlainTextEdit(parent)
{
    QFont monoFont;
    monoFont.setFamily("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    monoFont.setPointSize(12);

    this->setFont(monoFont);

    QFontMetrics metrics(monoFont);
    this->setTabStopWidth(4 * metrics.width(' '));

}

void LogWindow::log(string msg) {
    this->moveCursor(QTextCursor::End);
    this->insertPlainText(QString::fromStdString(msg + '\n'));
    this->moveCursor(QTextCursor::End);
}

void LogWindow::log(LogInfo info, string msg) {
    if(info.label == "") {
        info.label = "ERROR";
    }

    string output = info.label;
    if(info.first_line != 0 && info.last_line != 0) {
        if(info.first_line == info.last_line) {
            output += " at line " + to_string(info.first_line);
        } else {
            output += " at lines " + to_string(info.first_line) + "-" + to_string(info.last_line);
        }
    }
    output += ": " + msg;

    this->log(output);
}

void LogWindow::clear() {
    this->setPlainText("");
}
