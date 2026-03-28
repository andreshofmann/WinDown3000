#include "Editor.h"
#include "Preferences.h"

#include <QKeyEvent>
#include <QMimeData>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTimer>
#include <QPalette>

Editor::Editor(Preferences *prefs, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_prefs(prefs)
    , m_highlighter(new MarkdownHighlighter(prefs, document()))
    , m_wordCountTimer(new QTimer(this))
{
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    m_wordCountTimer->setSingleShot(true);
    m_wordCountTimer->setInterval(300);
    connect(m_wordCountTimer, &QTimer::timeout, this, &Editor::updateWordCount);
    connect(this, &QPlainTextEdit::textChanged, this, [this]() {
        m_wordCountTimer->start();
    });

    applyPreferences();
}

void Editor::applyTheme(const EditorTheme &theme)
{
    m_highlighter->setTheme(theme);

    QPalette pal = palette();
    pal.setColor(QPalette::Base, theme.background);
    pal.setColor(QPalette::Text, theme.foreground);
    pal.setColor(QPalette::Highlight, theme.selectionBackground);
    pal.setColor(QPalette::HighlightedText, theme.selectionForeground);
    setPalette(pal);

    // Caret color via stylesheet (QPalette doesn't support caret color)
    setStyleSheet(QStringLiteral(
        "QPlainTextEdit { selection-background-color: %1; }")
        .arg(theme.selectionBackground.name()));
}

void Editor::applyPreferences()
{
    setFont(m_prefs->editorFont());
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    // Line spacing — applied via default block format (matches MacDown's NSParagraphStyle)
    QTextBlockFormat blockFmt;
    blockFmt.setLineHeight(m_prefs->editorLineSpacing(), QTextBlockFormat::LineDistanceHeight);
    QTextCursor cur = textCursor();
    cur.select(QTextCursor::Document);
    cur.mergeBlockFormat(blockFmt);

    // Margins (matches MacDown's textContainerInset)
    int h = m_prefs->editorHorizontalInset();
    int v = m_prefs->editorVerticalInset();
    setViewportMargins(h, v, h, 0);

    // Load and apply theme
    QString themePath = QStringLiteral(":/themes/%1.style").arg(m_prefs->editorStyleName());
    EditorTheme theme = EditorTheme::load(themePath);
    applyTheme(theme);
}

// ---------------------------------------------------------------------------
// Key handling
// ---------------------------------------------------------------------------
void Editor::keyPressEvent(QKeyEvent *event)
{
    // Tab / Shift+Tab for indent/unindent
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) {
        handleTab(event->modifiers() & Qt::ShiftModifier || event->key() == Qt::Key_Backtab);
        return;
    }

    // Enter: smart list continuation
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        handleReturn();
        return;
    }

    // Backspace: remove matching bracket
    if (event->key() == Qt::Key_Backspace) {
        handleBackspace();
        return;
    }

    // Auto-close brackets/quotes
    if (m_prefs->editorCompleteMatchingCharacters()) {
        QChar ch = event->text().isEmpty() ? QChar() : event->text().at(0);
        if (ch == '(' || ch == '[' || ch == '{') {
            QChar closing = (ch == '(') ? ')' : (ch == '[') ? ']' : '}';
            insertMatchingChar(ch, closing);
            return;
        }
        if (ch == '"' || ch == '\'' || ch == '`') {
            QTextCursor cur = textCursor();
            if (cur.hasSelection()) {
                wrapSelection(QString(ch), QString(ch));
                return;
            }
            // Check if next char is the same closing char — skip over it
            if (!cur.atEnd()) {
                QChar next = document()->characterAt(cur.position());
                if (next == ch) {
                    cur.movePosition(QTextCursor::Right);
                    setTextCursor(cur);
                    return;
                }
            }
            insertMatchingChar(ch, ch);
            return;
        }
        // Skip over closing brackets
        if (ch == ')' || ch == ']' || ch == '}') {
            QTextCursor cur = textCursor();
            if (!cur.atEnd()) {
                QChar next = document()->characterAt(cur.position());
                if (next == ch) {
                    cur.movePosition(QTextCursor::Right);
                    setTextCursor(cur);
                    return;
                }
            }
        }
    }

    // Bold: Ctrl+B
    if (event->key() == Qt::Key_B && (event->modifiers() & Qt::ControlModifier)) {
        wrapSelection("**", "**");
        return;
    }
    // Italic: Ctrl+I
    if (event->key() == Qt::Key_I && (event->modifiers() & Qt::ControlModifier)) {
        wrapSelection("*", "*");
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void Editor::handleReturn()
{
    QTextCursor cur = textCursor();
    QString line = cur.block().text();

    // Check for list continuation
    static QRegularExpression unorderedRe(R"(^(\s*)([-*+])\s)");
    static QRegularExpression orderedRe(R"(^(\s*)(\d+)\.\s)");
    static QRegularExpression taskRe(R"(^(\s*[-*+]\s+)\[[ xX]\]\s)");
    static QRegularExpression blockquoteRe(R"(^(\s*>+\s*))");

    QRegularExpressionMatch match;

    // Empty list item → remove the bullet and stop the list
    static QRegularExpression emptyListRe(R"(^(\s*[-*+]\s+(\[[ xX]\]\s+)?)$)");
    static QRegularExpression emptyOrderedRe(R"(^(\s*\d+\.\s+)$)");
    if ((match = emptyListRe.match(line)).hasMatch() ||
        (match = emptyOrderedRe.match(line)).hasMatch()) {
        cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cur.removeSelectedText();
        cur.insertText("\n");
        setTextCursor(cur);
        return;
    }

    // Task list continuation
    if ((match = taskRe.match(line)).hasMatch()) {
        cur.insertText("\n" + match.captured(1) + "[ ] ");
        setTextCursor(cur);
        return;
    }

    // Ordered list continuation with auto-increment
    if ((match = orderedRe.match(line)).hasMatch()) {
        QString indent = match.captured(1);
        int num = match.captured(2).toInt();
        if (m_prefs->editorAutoIncrementNumberedLists())
            num++;
        cur.insertText(QStringLiteral("\n%1%2. ").arg(indent).arg(num));
        setTextCursor(cur);
        return;
    }

    // Unordered list continuation
    if ((match = unorderedRe.match(line)).hasMatch()) {
        cur.insertText(QStringLiteral("\n%1%2 ").arg(match.captured(1), match.captured(2)));
        setTextCursor(cur);
        return;
    }

    // Blockquote continuation
    if ((match = blockquoteRe.match(line)).hasMatch()) {
        cur.insertText("\n" + match.captured(1));
        setTextCursor(cur);
        return;
    }

    QPlainTextEdit::keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
}

void Editor::handleTab(bool shift)
{
    QTextCursor cur = textCursor();

    if (cur.hasSelection()) {
        // Indent/unindent selected lines
        int start = cur.selectionStart();
        int end = cur.selectionEnd();
        cur.setPosition(start);
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.setPosition(end, QTextCursor::KeepAnchor);
        cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        QString selected = cur.selectedText();
        // QTextCursor uses unicode paragraph separator
        QStringList lines = selected.split(QChar(0x2029));

        for (int i = 0; i < lines.size(); i++) {
            if (shift) {
                if (lines[i].startsWith("    "))
                    lines[i] = lines[i].mid(4);
                else if (lines[i].startsWith("\t"))
                    lines[i] = lines[i].mid(1);
            } else {
                lines[i] = "    " + lines[i];
            }
        }

        cur.insertText(lines.join(QChar(0x2029)));
    } else {
        if (shift) return;
        if (m_prefs->editorConvertTabs())
            cur.insertText("    ");
        else
            cur.insertText("\t");
    }
}

void Editor::toggleBold()
{
    toggleWrap("**", "**");
}

void Editor::toggleItalic()
{
    toggleWrap("*", "*");
}

void Editor::indentSelection()
{
    handleTab(false);
}

void Editor::unindentSelection()
{
    handleTab(true);
}

void Editor::handleBackspace()
{
    if (m_prefs->editorCompleteMatchingCharacters()) {
        QTextCursor cur = textCursor();
        if (!cur.atStart() && !cur.atEnd() && !cur.hasSelection()) {
            QChar prev = document()->characterAt(cur.position() - 1);
            QChar next = document()->characterAt(cur.position());
            bool isMatchingPair = (prev == '(' && next == ')') ||
                                  (prev == '[' && next == ']') ||
                                  (prev == '{' && next == '}') ||
                                  (prev == '"' && next == '"') ||
                                  (prev == '\'' && next == '\'') ||
                                  (prev == '`' && next == '`');
            if (isMatchingPair) {
                cur.deleteChar();       // delete closing
                cur.deletePreviousChar(); // delete opening
                setTextCursor(cur);
                return;
            }
        }
    }

    QPlainTextEdit::keyPressEvent(
        new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier));
}

void Editor::insertMatchingChar(QChar opening, QChar closing)
{
    QTextCursor cur = textCursor();
    if (cur.hasSelection()) {
        wrapSelection(QString(opening), QString(closing));
    } else {
        cur.insertText(QString(opening) + QString(closing));
        cur.movePosition(QTextCursor::Left);
        setTextCursor(cur);
    }
}

bool Editor::shouldAutoClose(QChar) const
{
    return m_prefs->editorCompleteMatchingCharacters();
}

void Editor::toggleWrap(const QString &before, const QString &after)
{
    QTextCursor cur = textCursor();
    if (cur.hasSelection()) {
        int start = cur.selectionStart();
        int end = cur.selectionEnd();
        QString selected = cur.selectedText();

        // Check if selection is already wrapped — unwrap it
        if (selected.startsWith(before) && selected.endsWith(after) &&
            selected.length() > before.length() + after.length()) {
            QString inner = selected.mid(before.length(),
                                         selected.length() - before.length() - after.length());
            cur.insertText(inner);
            cur.setPosition(start);
            cur.setPosition(start + inner.length(), QTextCursor::KeepAnchor);
            setTextCursor(cur);
            return;
        }

        // Check if text around the selection has the markers — unwrap
        QString fullText = toPlainText();
        int beforeStart = start - before.length();
        int afterEnd = end + after.length();
        if (beforeStart >= 0 && afterEnd <= fullText.length() &&
            fullText.mid(beforeStart, before.length()) == before &&
            fullText.mid(end, after.length()) == after) {
            cur.setPosition(beforeStart);
            cur.setPosition(afterEnd, QTextCursor::KeepAnchor);
            cur.insertText(selected);
            cur.setPosition(beforeStart);
            cur.setPosition(beforeStart + selected.length(), QTextCursor::KeepAnchor);
            setTextCursor(cur);
            return;
        }
    }
    // Not already wrapped — wrap it
    wrapSelection(before, after);
}

void Editor::wrapSelection(const QString &before, const QString &after)
{
    QTextCursor cur = textCursor();
    if (!cur.hasSelection()) {
        cur.insertText(before + after);
        cur.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, after.length());
        setTextCursor(cur);
        return;
    }
    int start = cur.selectionStart();
    int end = cur.selectionEnd();
    QString selected = cur.selectedText();
    cur.insertText(before + selected + after);
    // Re-select the wrapped text
    cur.setPosition(start + before.length());
    cur.setPosition(start + before.length() + selected.length(), QTextCursor::KeepAnchor);
    setTextCursor(cur);
}

void Editor::insertFromMimeData(const QMimeData *source)
{
    // Smart paste: if clipboard contains a URL and there's a selection, make a link
    if (source->hasText() && textCursor().hasSelection()) {
        QString text = source->text().trimmed();
        static QRegularExpression urlRe(R"(^https?://.+)");
        if (urlRe.match(text).hasMatch()) {
            wrapSelection("[", QStringLiteral("](%1)").arg(text));
            return;
        }
    }
    QPlainTextEdit::insertFromMimeData(source);
}

// ---------------------------------------------------------------------------
// Word count
// ---------------------------------------------------------------------------
int Editor::wordCount() const
{
    QString text = toPlainText();
    if (text.isEmpty()) return 0;
    static QRegularExpression ws(R"(\s+)");
    return text.split(ws, Qt::SkipEmptyParts).count();
}

int Editor::charCount() const
{
    return toPlainText().length();
}

void Editor::updateWordCount()
{
    emit wordCountChanged(wordCount(), charCount());
}
