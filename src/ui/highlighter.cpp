#include "highlighter.h"

Highlighter::Highlighter(QTextDocument *parent): QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    QTextCharFormat keywordFormat;
    QTextCharFormat literalFormat;
    QTextCharFormat typeFormat;

    keywordFormat.setForeground(QColor(0, 153, 153));
    keywordFormat.setFontWeight(70);
    QStringList keywordPatterns;
    keywordPatterns << "\\buse\\b" << "\\ballocate\\b" << "\\bdraw\\b" << "\\bto\\b" << "\\busing\\b" << "\\btrue\\b" << "\\bfalse\\b" << "\\band\\b" << "\\bor\\b" << "\\bfunc\\b" << "\\bprint\\b" << "\\bimport\\b";

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

    typeFormat.setForeground(QColor(128, 0, 255));
    typeFormat.setFontWeight(60);
    QStringList typePatterns;
    typePatterns << "\\bint\\b" << "\\bfloat\\b" << "\\bbool\\b" << "\\bbuffer\\b" << "\\btexture2D\\b" << "\\bvec2\\b" << "\\bvec3\\b" << "\\bvec4\\b" << "\\bmat2\\b" << "\\bmat3\\b" << "\\bmat4\\b";
    foreach(const QString &pattern, typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = typeFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setForeground(Qt::darkGray);
    singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("//.*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void Highlighter::highlightBlock(const QString &text) {
    foreach(const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while(matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if(previousBlockState() != 1) {
        startIndex = text.indexOf(commentStartExpression);
    }

    while(startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if(endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, singleLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
