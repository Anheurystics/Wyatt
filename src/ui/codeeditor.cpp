﻿#include "codeeditor.h"

std::map<string, FuncDef::ptr> CodeEditor::autocomplete_functions;

CodeEditor::CodeEditor(QWidget *parent = 0): QPlainTextEdit(parent)
{
    monoFont.setFamily("Monospace");
    monoFont.insertSubstitution("Monospace", "Courier New");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    monoFont.setPointSize(11);

    setFont(monoFont);

    QFontMetrics metrics(monoFont);
    setTabStopWidth(4 * metrics.width(' '));

    lineNumberArea = new LineNumberArea(this);
    functionHintBox = new FunctionHintBox(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::updateFunctionHint() {
    string bef = toPlainText().left(textCursor().position()).toStdString();
    reverse(bef.begin(), bef.end());
    QString before = QString::fromStdString(bef);
    QString filtered = before.remove(QRegExp(R"(\)[\w\d\s,_]*\([\w\d_]*\s+cnuf)")).remove(QRegExp(R"("[\(\)\w\d\s._,]*")")).remove(QRegExp(R"([\(\)\w\d\s\._,]*")"));
    QRegExp re(R"(([\w\d\s\.,_]*)\(([\w\d_]+))");
    re.indexIn(filtered); 
    autocompleteText = "";

    unsigned int typed_args = re.cap(1).count(",");

    string function_name = re.cap(2).toStdString();
    reverse(function_name.begin(), function_name.end());

    auto functions = CodeEditor::autocomplete_functions;
    FuncDef::ptr func = nullptr;
    if(functions.find(function_name) != functions.end()) {
        func = functions[function_name];
    }

    if(before.count("(") > before.count(")") && !filtered.contains("cnuf") && func != nullptr && typed_args < func->params->list.size()) {
        autocompleteText = QString::fromStdString(func->ident->name + "(");
        for(unsigned int i = 0; i < func->params->list.size(); ++i) {
            Decl::ptr decl = func->params->list[i];
            autocompleteText += QString::fromStdString(decl->datatype->name + " " + decl->ident->name);
            if(i == typed_args) {
                currentParam = QString::fromStdString(decl->datatype->name + " " + decl->ident->name);
            }
            if(i + 1 != func->params->list.size()) {
                autocompleteText += ", ";
            }
        }
        autocompleteText += ")";
    }
}

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    QString toInsert = "";
    QPlainTextEdit::keyPressEvent(event);
    if(event->key() == Qt::Key_Return) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor, 1);

        int oldPos = cursor.position();
        cursor.select(QTextCursor::LineUnderCursor);
        QString prevLine = cursor.selectedText();
        QRegExp preTab(R"(^(\s*))");
        preTab.indexIn(prevLine);
        toInsert = preTab.capturedTexts().at(0);
        cursor.setPosition(oldPos);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 1);

        cursor.select(QTextCursor::LineUnderCursor);
        QString currLine = cursor.selectedText();

        if(prevLine.endsWith("{") && !currLine.startsWith("}")) {
            toInsert += "\t";
        }

        cursor.select(QTextCursor::LineUnderCursor);
        QString selectedText = cursor.selectedText().trimmed();

        oldPos = cursor.positionInBlock();
        cursor.removeSelectedText();
        cursor.insertText(toInsert + selectedText);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, oldPos);
        setTextCursor(cursor);
    }
    if(event->key() == Qt::Key_BraceLeft) {
        QTextCursor cursor = textCursor();
        cursor.insertText("}");
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        setTextCursor(cursor);
    }

    updateFunctionHint();
    functionHintBox->update();
}

void CodeEditor::mousePressEvent(QMouseEvent* e) {
    QPlainTextEdit::mousePressEvent(e);
    if(e->button() == Qt::LeftButton) {
        updateFunctionHint();
    }
    functionHintBox->update();
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while(max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if(dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if(rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if(!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(180);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    while(block.isValid() && top <= event->rect().bottom()) {
        if(block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.setFont(monoFont);
            painter.drawText(-3, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::functionHintBoxPaintEvent(QPaintEvent*) {
    QPainter painter(functionHintBox);

    if(autocompleteText != "") {
        QRect eventRect = cursorRect();
        painter.setPen(Qt::black);
        painter.setFont(monoFont);
        eventRect.translate(30, -fontMetrics().height());
        eventRect.setWidth(fontMetrics().width(autocompleteText) + 10);
        eventRect.setHeight(fontMetrics().height() + 5);
        painter.fillRect(eventRect, QColor(Qt::gray).lighter(150));
        painter.drawRect(eventRect);
        eventRect.translate(5, 4);
        painter.drawText(eventRect, autocompleteText);
        monoFont.setBold(true);
        painter.setFont(monoFont);
        int offset = autocompleteText.indexOf(currentParam);
        if(offset != -1) {
            eventRect.setX(eventRect.x() + (offset * fontMetrics().width(" ")));
            painter.drawText(eventRect, currentParam);
        }
        monoFont.setBold(false);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    functionHintBox->setGeometry(cr);
}
