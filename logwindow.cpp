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

void LogWindow::log(std::string msg) {
    this->moveCursor(QTextCursor::End);
    this->insertPlainText(QString::fromStdString(msg + '\n'));
    this->moveCursor(QTextCursor::End);
}

void LogWindow::clear() {
    this->setPlainText("");
}
