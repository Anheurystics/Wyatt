#ifndef CODEEDITOR_H 
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include <QPainter>
#include <QTextBlock>
#include <QFileInfo>
#include <iostream>
#include <map>

#include "nodes.h"

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;
class FunctionHintBox;

class CodeEditor: public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void functionHintBoxPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    QFileInfo fileInfo;
    QFont monoFont;

   static std::map<string, FuncDef_ptr> autocomplete_functions;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent*) override;

private slots:
    void updateLineNumberAreaWidth(int);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    LineNumberArea* lineNumberArea;
    FunctionHintBox* functionHintBox;

    QString autocompleteText;
    QString currentParam;
};

class LineNumberArea : public QWidget
{
    public:
        LineNumberArea(CodeEditor* codeEditor): QWidget(codeEditor) {
            this->codeEditor = codeEditor;
        }

        QSize sizeHint() const override {
            return QSize(codeEditor->lineNumberAreaWidth(), 0);
        }

    protected:
        void paintEvent(QPaintEvent *event) {
            codeEditor->lineNumberAreaPaintEvent(event);
        }

    private:
        CodeEditor* codeEditor;
};

class FunctionHintBox : public QWidget
{
    public:
        FunctionHintBox(CodeEditor* codeEditor): QWidget(codeEditor) {
            this->codeEditor = codeEditor;
            setAttribute(Qt::WA_TransparentForMouseEvents);
        }

    protected:
        void paintEvent(QPaintEvent *event) {
            codeEditor->functionHintBoxPaintEvent(event);
        }

    private:
        CodeEditor* codeEditor;
};

#endif // CODEEDITOR_H
