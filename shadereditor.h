#ifndef SHADEREDITOR_H
#define SHADEREDITOR_H

#include <QPlainTextEdit>
#include <iostream>

class ShaderEditor: public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit ShaderEditor(QWidget *parent);
    void setType(std::string);
    std::string getType();
private:
    std::string type;
};

#endif // SHADEREDITOR_H
