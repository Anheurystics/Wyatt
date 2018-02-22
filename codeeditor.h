#ifndef CODEEDITOR_H 
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include <QPainter>
#include <QTextBlock>
#include <QFileInfo>
#include <iostream>

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;

class CodeEditor: public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    QFileInfo fileInfo;
    QFont monoFont;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    LineNumberArea* lineNumberArea;
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



#endif // CODEEDITOR_H
