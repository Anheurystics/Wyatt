#include "highlighter.h"

Highlighter::Highlighter(QTextDocument *parent): QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    QTextCharFormat keywordFormat;
    QTextCharFormat literalFormat;
    QTextCharFormat singleLineCommentFormat;

    keywordFormat.setForeground(QColor(0, 153, 153));
    keywordFormat.setFontWeight(70);
    QStringList keywordPatterns;
    keywordPatterns << "\\buse\\b" << "\\ballocate\\b" << "\\bdraw\\b" << "\\bto\\b" << "\\busing\\b" << "\\btrue\\b" << "\\bfalse\\b" << "\\band\\b" << "\\bor\\b" << "\\bfunc\\b" << "\\bprint\\b";

    foreach(const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    literalFormat.setForeground(QColor(230, 96, 0));

    QStringList literalPatterns;
    literalPatterns << "\\b\\d+\\b" << "\\b\\d+\\.\\d+\\b" << R"(\"(\\.|[^\\"])*\")";
    foreach(const QString &pattern, literalPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = literalFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setForeground(Qt::darkGray);
    singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("//.*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
}

void Highlighter::highlightBlock(const QString &text) {
    foreach(const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while(matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
