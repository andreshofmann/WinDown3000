#include <gtest/gtest.h>
#include <QApplication>
#include <QPlainTextEdit>
#include <QTest>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include "FindReplaceDialog.h"

static int argc = 1;
static char *argv[] = {(char *)"test_find_replace"};

class FindReplaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        editor = new QPlainTextEdit();
        dialog = new FindReplaceDialog(editor);
        editor->show();
    }
    void TearDown() override {
        delete dialog;
        delete editor;
    }

    QLineEdit *findField() { return dialog->findChild<QLineEdit *>(); }
    QLineEdit *replaceField() {
        auto fields = dialog->findChildren<QLineEdit *>();
        return fields.size() > 1 ? fields[1] : nullptr;
    }
    QPushButton *findButton(const QString &text) {
        for (auto *btn : dialog->findChildren<QPushButton *>()) {
            if (btn->text().contains(text, Qt::CaseInsensitive))
                return btn;
        }
        return nullptr;
    }
    QCheckBox *checkBox(const QString &text) {
        for (auto *cb : dialog->findChildren<QCheckBox *>()) {
            if (cb->text().contains(text, Qt::CaseInsensitive))
                return cb;
        }
        return nullptr;
    }

    QPlainTextEdit *editor;
    FindReplaceDialog *dialog;
};

// ---------------------------------------------------------------------------
// Find
// ---------------------------------------------------------------------------
TEST_F(FindReplaceTest, FindNextSelectsMatch) {
    editor->setPlainText("Hello world hello");
    dialog->show();
    findField()->setText("hello");

    auto *btn = findButton("Next");
    ASSERT_NE(btn, nullptr);
    btn->click();

    EXPECT_TRUE(editor->textCursor().hasSelection());
    EXPECT_EQ(editor->textCursor().selectedText().toLower(), "hello");
}

TEST_F(FindReplaceTest, FindWrapsAround) {
    editor->setPlainText("one two one");
    QTextCursor cur = editor->textCursor();
    cur.movePosition(QTextCursor::End);
    editor->setTextCursor(cur);

    dialog->show();
    findField()->setText("one");

    auto *btn = findButton("Next");
    btn->click();

    // Should wrap to the beginning and find "one"
    EXPECT_TRUE(editor->textCursor().hasSelection());
    EXPECT_EQ(editor->textCursor().selectedText(), "one");
}

TEST_F(FindReplaceTest, FindPrevious) {
    editor->setPlainText("aaa bbb aaa");
    QTextCursor cur = editor->textCursor();
    cur.movePosition(QTextCursor::End);
    editor->setTextCursor(cur);

    dialog->show();
    findField()->setText("aaa");

    auto *btn = findButton("Previous");
    ASSERT_NE(btn, nullptr);
    btn->click();

    EXPECT_TRUE(editor->textCursor().hasSelection());
    EXPECT_EQ(editor->textCursor().selectedText(), "aaa");
}

TEST_F(FindReplaceTest, FindCaseSensitive) {
    editor->setPlainText("Hello hello HELLO");
    dialog->show();
    findField()->setText("hello");

    auto *cs = checkBox("case");
    ASSERT_NE(cs, nullptr);
    cs->setChecked(true);

    auto *btn = findButton("Next");
    btn->click();

    EXPECT_TRUE(editor->textCursor().hasSelection());
    EXPECT_EQ(editor->textCursor().selectedText(), "hello");
}

TEST_F(FindReplaceTest, FindNoMatch) {
    editor->setPlainText("Hello world");
    dialog->show();
    findField()->setText("xyz");

    auto *btn = findButton("Next");
    btn->click();

    EXPECT_FALSE(editor->textCursor().hasSelection());
}

// ---------------------------------------------------------------------------
// Replace
// ---------------------------------------------------------------------------
TEST_F(FindReplaceTest, ReplaceCurrentMatch) {
    editor->setPlainText("old text here");
    dialog->show();
    findField()->setText("old");
    replaceField()->setText("new");

    // First find
    auto *findBtn = findButton("Next");
    findBtn->click();

    // Then replace
    auto *replBtn = findButton("Replace");
    ASSERT_NE(replBtn, nullptr);
    replBtn->click();

    EXPECT_TRUE(editor->toPlainText().contains("new text here"));
    EXPECT_FALSE(editor->toPlainText().contains("old text here"));
}

TEST_F(FindReplaceTest, ReplaceAll) {
    editor->setPlainText("foo bar foo baz foo");
    dialog->show();
    findField()->setText("foo");
    replaceField()->setText("qux");

    auto *btn = findButton("All");
    ASSERT_NE(btn, nullptr);
    btn->click();

    EXPECT_EQ(editor->toPlainText(), "qux bar qux baz qux");
}

TEST_F(FindReplaceTest, ReplaceAllCaseSensitive) {
    editor->setPlainText("Foo foo FOO");
    dialog->show();
    findField()->setText("foo");
    replaceField()->setText("bar");

    auto *cs = checkBox("case");
    cs->setChecked(true);

    auto *btn = findButton("All");
    btn->click();

    EXPECT_EQ(editor->toPlainText(), "Foo bar FOO");
}

TEST_F(FindReplaceTest, ReplaceAllNoMatch) {
    editor->setPlainText("nothing here");
    dialog->show();
    findField()->setText("xyz");
    replaceField()->setText("abc");

    auto *btn = findButton("All");
    btn->click();

    EXPECT_EQ(editor->toPlainText(), "nothing here");
}

// ---------------------------------------------------------------------------
// Regex
// ---------------------------------------------------------------------------
TEST_F(FindReplaceTest, RegexFind) {
    editor->setPlainText("abc 123 def 456");
    dialog->show();
    findField()->setText("\\d+");

    auto *regexCb = checkBox("Regex");
    ASSERT_NE(regexCb, nullptr);
    regexCb->setChecked(true);

    auto *btn = findButton("Next");
    btn->click();

    EXPECT_TRUE(editor->textCursor().hasSelection());
    EXPECT_EQ(editor->textCursor().selectedText(), "123");
}

TEST_F(FindReplaceTest, RegexReplaceAll) {
    editor->setPlainText("2024-01-15 and 2024-02-20");
    dialog->show();
    findField()->setText("\\d{4}-\\d{2}-\\d{2}");
    replaceField()->setText("DATE");

    auto *regexCb = checkBox("Regex");
    regexCb->setChecked(true);

    auto *btn = findButton("All");
    btn->click();

    EXPECT_EQ(editor->toPlainText(), "DATE and DATE");
}

// ---------------------------------------------------------------------------
// UI state
// ---------------------------------------------------------------------------
TEST_F(FindReplaceTest, ButtonsDisabledWhenEmpty) {
    dialog->show();
    findField()->clear();

    auto *btn = findButton("Next");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isEnabled());
}

TEST_F(FindReplaceTest, ButtonsEnabledWithText) {
    dialog->show();
    findField()->setText("something");

    auto *btn = findButton("Next");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isEnabled());
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
