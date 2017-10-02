#ifndef SHADEREDITOR_H
#define SHADEREDITOR_H

#include <QPlainTextEdit>
#include <iostream>

class CodeEditor: public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent);
    void setType(std::string);
    std::string getType();
private:
    std::string type;
};

#endif // SHADEREDITOR_H
