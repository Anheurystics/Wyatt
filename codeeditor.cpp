#include "codeeditor.h"

CodeEditor::CodeEditor(QWidget *parent = 0): QPlainTextEdit(parent)
{
    type = "";

    QFont monoFont;
    monoFont.setFamily("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    monoFont.setPointSize(12);

    this->setFont(monoFont);

    QFontMetrics metrics(monoFont);
    this->setTabStopWidth(4 * metrics.width(' '));
}

void CodeEditor::setType(std::string _type)
{
    type = _type;
}

std::string CodeEditor::getType()
{
    return type;
}