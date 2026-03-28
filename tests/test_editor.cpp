#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QTextCursor>
#include "Editor.h"
#include "Preferences.h"

// Requires QApplication for widget tests
static int argc = 1;
static char *argv[] = {(char *)"test_editor"};

class EditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        prefs = new Preferences();
        editor = new Editor(prefs);
        editor->show();
    }
    void TearDown() override {
        delete editor;
        delete prefs;
    }
    Preferences *prefs;
    Editor *editor;

    void typeText(const QString &text) {
        for (const QChar &ch : text) {
            QTest::keyClick(editor, ch.toLatin1());
        }
    }

    void setCursorPosition(int pos) {
        QTextCursor cur = editor->textCursor();
        cur.setPosition(pos);
        editor->setTextCursor(cur);
    }
};

// ---------------------------------------------------------------------------
// Basic operations
// ---------------------------------------------------------------------------
TEST_F(EditorTest, InitiallyEmpty) {
    EXPECT_TRUE(editor->toPlainText().isEmpty());
}

TEST_F(EditorTest, SetText) {
    editor->setPlainText("Hello world");
    EXPECT_EQ(editor->toPlainText(), "Hello world");
}

// ---------------------------------------------------------------------------
// Word count
// ---------------------------------------------------------------------------
TEST_F(EditorTest, WordCountEmpty) {
    EXPECT_EQ(editor->wordCount(), 0);
}

TEST_F(EditorTest, WordCountSingleWord) {
    editor->setPlainText("hello");
    EXPECT_EQ(editor->wordCount(), 1);
}

TEST_F(EditorTest, WordCountMultipleWords) {
    editor->setPlainText("one two three four");
    EXPECT_EQ(editor->wordCount(), 4);
}

TEST_F(EditorTest, WordCountMultipleLines) {
    editor->setPlainText("first line\nsecond line\nthird");
    EXPECT_EQ(editor->wordCount(), 5);
}

TEST_F(EditorTest, CharCount) {
    editor->setPlainText("Hello");
    EXPECT_EQ(editor->charCount(), 5);
}

TEST_F(EditorTest, CharCountWithSpaces) {
    editor->setPlainText("a b c");
    EXPECT_EQ(editor->charCount(), 5);
}

// ---------------------------------------------------------------------------
// Toggle bold/italic
// ---------------------------------------------------------------------------
TEST_F(EditorTest, ToggleBoldNoSelection) {
    editor->setPlainText("");
    editor->toggleBold();
    EXPECT_EQ(editor->toPlainText(), "****");
}

TEST_F(EditorTest, ToggleItalicNoSelection) {
    editor->setPlainText("");
    editor->toggleItalic();
    EXPECT_EQ(editor->toPlainText(), "**");
}

TEST_F(EditorTest, ToggleBoldWithSelection) {
    editor->setPlainText("hello");
    QTextCursor cur = editor->textCursor();
    cur.select(QTextCursor::Document);
    editor->setTextCursor(cur);
    editor->toggleBold();
    EXPECT_EQ(editor->toPlainText(), "**hello**");
}

TEST_F(EditorTest, ToggleItalicWithSelection) {
    editor->setPlainText("hello");
    QTextCursor cur = editor->textCursor();
    cur.select(QTextCursor::Document);
    editor->setTextCursor(cur);
    editor->toggleItalic();
    EXPECT_EQ(editor->toPlainText(), "*hello*");
}

// ---------------------------------------------------------------------------
// Bracket auto-completion
// ---------------------------------------------------------------------------
TEST_F(EditorTest, AutoCloseParen) {
    prefs->setEditorCompleteMatchingCharacters(true);
    editor->setPlainText("");
    QTest::keyClick(editor, '(');
    EXPECT_EQ(editor->toPlainText(), "()");
}

TEST_F(EditorTest, AutoCloseBracket) {
    prefs->setEditorCompleteMatchingCharacters(true);
    editor->setPlainText("");
    QTest::keyClick(editor, '[');
    EXPECT_EQ(editor->toPlainText(), "[]");
}

TEST_F(EditorTest, AutoCloseBrace) {
    prefs->setEditorCompleteMatchingCharacters(true);
    editor->setPlainText("");
    QTest::keyClick(editor, '{');
    EXPECT_EQ(editor->toPlainText(), "{}");
}

TEST_F(EditorTest, AutoCloseQuote) {
    prefs->setEditorCompleteMatchingCharacters(true);
    editor->setPlainText("");
    QTest::keyClick(editor, '"');
    EXPECT_EQ(editor->toPlainText(), "\"\"");
}

TEST_F(EditorTest, AutoCloseBacktick) {
    prefs->setEditorCompleteMatchingCharacters(true);
    editor->setPlainText("");
    QTest::keyClick(editor, '`');
    EXPECT_EQ(editor->toPlainText(), "``");
}

TEST_F(EditorTest, SkipOverClosingParen) {
    prefs->setEditorCompleteMatchingCharacters(true);
    editor->setPlainText("");
    QTest::keyClick(editor, '(');
    // Cursor is between ( and )
    QTest::keyClick(editor, ')');
    // Should skip over the ), not insert another
    EXPECT_EQ(editor->toPlainText(), "()");
}

TEST_F(EditorTest, NoAutoCloseWhenDisabled) {
    prefs->setEditorCompleteMatchingCharacters(false);
    editor->setPlainText("");
    QTest::keyClick(editor, '(');
    EXPECT_EQ(editor->toPlainText(), "(");
}

// ---------------------------------------------------------------------------
// Tab handling
// ---------------------------------------------------------------------------
TEST_F(EditorTest, TabInsertsSpaces) {
    prefs->setEditorConvertTabs(true);
    editor->setPlainText("");
    QTest::keyClick(editor, Qt::Key_Tab);
    EXPECT_EQ(editor->toPlainText(), "    ");
}

TEST_F(EditorTest, TabInsertsTab) {
    prefs->setEditorConvertTabs(false);
    editor->setPlainText("");
    QTest::keyClick(editor, Qt::Key_Tab);
    EXPECT_EQ(editor->toPlainText(), "\t");
}

// ---------------------------------------------------------------------------
// List continuation
// ---------------------------------------------------------------------------
TEST_F(EditorTest, UnorderedListContinuation) {
    editor->setPlainText("- item one");
    QTextCursor cur = editor->textCursor();
    cur.movePosition(QTextCursor::End);
    editor->setTextCursor(cur);
    QTest::keyClick(editor, Qt::Key_Return);
    EXPECT_TRUE(editor->toPlainText().contains("- item one\n- "));
}

TEST_F(EditorTest, OrderedListContinuation) {
    prefs->setEditorAutoIncrementNumberedLists(true);
    editor->setPlainText("1. item one");
    QTextCursor cur = editor->textCursor();
    cur.movePosition(QTextCursor::End);
    editor->setTextCursor(cur);
    QTest::keyClick(editor, Qt::Key_Return);
    EXPECT_TRUE(editor->toPlainText().contains("2. "));
}

TEST_F(EditorTest, EmptyListItemRemovesBullet) {
    editor->setPlainText("- ");
    QTextCursor cur = editor->textCursor();
    cur.movePosition(QTextCursor::End);
    editor->setTextCursor(cur);
    QTest::keyClick(editor, Qt::Key_Return);
    // Should remove the bullet and leave a blank line
    EXPECT_FALSE(editor->toPlainText().contains("- "));
}

// ---------------------------------------------------------------------------
// Word count signal
// ---------------------------------------------------------------------------
TEST_F(EditorTest, WordCountSignalEmitted) {
    QSignalSpy spy(editor, &Editor::wordCountChanged);
    editor->setPlainText("hello world");
    // Wait for the debounced timer
    QTest::qWait(500);
    EXPECT_GE(spy.count(), 1);
    auto args = spy.last();
    EXPECT_EQ(args.at(0).toInt(), 2); // 2 words
    EXPECT_EQ(args.at(1).toInt(), 11); // 11 chars
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
