#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>
#include "ThemeParser.h"

class Preferences;

/// Syntax highlighter for the Markdown editor.
/// Applies formatting from the active .style theme to headings, emphasis,
/// code, links, images, lists, blockquotes, etc.
class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(Preferences *prefs, QTextDocument *parent = nullptr);

    void setTheme(const EditorTheme &theme);
    const EditorTheme &theme() const { return m_theme; }

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QString formatKey;       // key into EditorTheme::formats
    };

    void buildRules();
    void applyRule(const QString &text, const Rule &rule);

    QVector<Rule> m_rules;
    EditorTheme m_theme;
    Preferences *m_prefs;

    // Multi-line state for fenced code blocks
    QRegularExpression m_fenceOpen;
    QRegularExpression m_fenceClose;
};
