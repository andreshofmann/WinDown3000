#pragma once

#include <QPlainTextEdit>
#include "MarkdownHighlighter.h"

class Preferences;

/// Markdown editor widget with syntax highlighting, auto-completion,
/// bracket pairing, and smart list continuation.
class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(Preferences *prefs, QWidget *parent = nullptr);

    void applyTheme(const EditorTheme &theme);
    void applyPreferences();
    int wordCount() const;
    int charCount() const;

    MarkdownHighlighter *highlighter() const { return m_highlighter; }

    // Public formatting actions (called from MainWindow menus/toolbar)
    void toggleBold();
    void toggleItalic();
    void indentSelection();
    void unindentSelection();

signals:
    void wordCountChanged(int words, int chars);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    void handleReturn();
    void handleTab(bool shift);
    void handleBackspace();
    void insertMatchingChar(QChar opening, QChar closing);
    bool shouldAutoClose(QChar ch) const;
    void updateWordCount();
    void toggleWrap(const QString &before, const QString &after);
    void wrapSelection(const QString &before, const QString &after);

    MarkdownHighlighter *m_highlighter;
    Preferences *m_prefs;
    QTimer *m_wordCountTimer;
};
