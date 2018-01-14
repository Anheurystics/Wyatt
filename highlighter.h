#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <SOIL/SOIL.h>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
	QTextCharFormat singleLineCommentFormat;
	QTextCharFormat quotationFormat;
	QTextCharFormat functionFormat;
};

#endif // HIGHLIGHTER_H
