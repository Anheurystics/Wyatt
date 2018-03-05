#include "codeeditor.h"

std::map<string, FuncDef_ptr> CodeEditor::autocomplete_functions;

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

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    QString toInsert = "";
    if(event->key() == Qt::Key_Return) {
        QTextCursor cursor = textCursor();
        //TODO: Better/faster way to do this?
        int oldPos = cursor.position();
        cursor.select(QTextCursor::LineUnderCursor);
        QString prevLine = cursor.selectedText();
        QRegExp preTab("^(\\s*)");
        preTab.indexIn(prevLine); // Needed for some reason
        toInsert = preTab.capturedTexts().at(0);
        cursor.setPosition(oldPos);
        if(prevLine.endsWith("{") && cursor.positionInBlock() != 0) {
            toInsert += "\t";
        }
    }
    QPlainTextEdit::keyPressEvent(event);
    if(event->key() == Qt::Key_Return) {
        QTextCursor cursor = textCursor();
        cursor.insertText(toInsert);
        cursor.movePosition(QTextCursor::EndOfLine);
    }
    if(event->key() == Qt::Key_BraceLeft) {
        QTextCursor cursor = textCursor();
        cursor.insertText("}");
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        setTextCursor(cursor);
    }

    //Only update function hint if user typed a character
    if(event->text().size() > 0) {
        QString before = toPlainText().left(textCursor().position());
        before = before.right(before.size() - before.lastIndexOf(QRegExp("(\\s)+"), before.lastIndexOf("(")) - 1);
        autocompleteText = "";
        if(before.indexOf("(") != -1 && before.indexOf(")") == -1) {
            int typed_args = before.count(",");
            string function_name = before.left(before.indexOf("(")).toStdString();
            auto functions = CodeEditor::autocomplete_functions;
            if(functions.find(function_name) != functions.end()) {
                FuncDef_ptr func =  functions[function_name];
                if(func != nullptr) {
                    autocompleteText = QString::fromStdString(func->ident->name + "(");
                    for(unsigned int i = 0; i < func->params->list.size(); ++i) {
                        Decl_ptr decl = func->params->list[i];
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
        }
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

void CodeEditor::updateLineNumberAreaWidth(int newBlockCount) {
    (void)newBlockCount;
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

void CodeEditor::functionHintBoxPaintEvent(QPaintEvent *e) {
    QPainter painter(functionHintBox);

    if(autocompleteText != "") {
        QRect eventRect = cursorRect();
        painter.setPen(Qt::black);
        painter.setFont(monoFont);
        eventRect.translate(30, -fontMetrics().height());
        eventRect.setWidth(fontMetrics().width(autocompleteText));
        eventRect.setHeight(fontMetrics().height());
        painter.fillRect(eventRect, QColor(Qt::lightGray).lighter(120));
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
