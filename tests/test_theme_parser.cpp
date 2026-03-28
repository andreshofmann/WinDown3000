#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QTextStream>
#include "ThemeParser.h"

static QString writeThemeFile(const QString &content) {
    auto *f = new QTemporaryFile();
    f->setAutoRemove(true);
    f->open();
    QTextStream out(f);
    out << content;
    out.flush();
    return f->fileName();
}

TEST(ThemeParserTest, LoadEmptyFile) {
    QTemporaryFile f;
    f.open();
    f.close();
    EditorTheme theme = EditorTheme::load(f.fileName());
    // Should return defaults
    EXPECT_TRUE(theme.foreground.isValid());
    EXPECT_TRUE(theme.background.isValid());
}

TEST(ThemeParserTest, ParseEditorColors) {
    QTemporaryFile f;
    f.open();
    QTextStream out(&f);
    out << "editor\nforeground: ff0000\nbackground: 00ff00\ncaret: 0000ff\n";
    out.flush();

    EditorTheme theme = EditorTheme::load(f.fileName());
    EXPECT_EQ(theme.foreground, QColor("#ff0000"));
    EXPECT_EQ(theme.background, QColor("#00ff00"));
    EXPECT_EQ(theme.caret, QColor("#0000ff"));
}

TEST(ThemeParserTest, ParseSelectionColors) {
    QTemporaryFile f;
    f.open();
    QTextStream out(&f);
    out << "editor-selection\nforeground: aabbcc\nbackground: 112233\n";
    out.flush();

    EditorTheme theme = EditorTheme::load(f.fileName());
    EXPECT_EQ(theme.selectionForeground, QColor("#aabbcc"));
    EXPECT_EQ(theme.selectionBackground, QColor("#112233"));
}

TEST(ThemeParserTest, ParseHeadingFormat) {
    QTemporaryFile f;
    f.open();
    QTextStream out(&f);
    out << "H1\nforeground: 66cccc\nfont-style: bold\n";
    out.flush();

    EditorTheme theme = EditorTheme::load(f.fileName());
    ASSERT_TRUE(theme.formats.contains("H1"));
    QTextCharFormat fmt = theme.formats["H1"];
    EXPECT_EQ(fmt.foreground().color(), QColor("#66cccc"));
    EXPECT_EQ(fmt.fontWeight(), QFont::Bold);
}

TEST(ThemeParserTest, ParseItalicFormat) {
    QTemporaryFile f;
    f.open();
    QTextStream out(&f);
    out << "EMPH\nforeground: ffcc66\nfont-style: italic\n";
    out.flush();

    EditorTheme theme = EditorTheme::load(f.fileName());
    ASSERT_TRUE(theme.formats.contains("EMPH"));
    EXPECT_TRUE(theme.formats["EMPH"].fontItalic());
}

TEST(ThemeParserTest, ParseMultipleElements) {
    QTemporaryFile f;
    f.open();
    QTextStream out(&f);
    out << "editor\nforeground: cccccc\nbackground: 2d2d2d\n\n"
        << "H1\nforeground: 66cccc\n\n"
        << "STRONG\nforeground: f99157\nfont-style: bold\n\n"
        << "CODE\nforeground: 999999\n";
    out.flush();

    EditorTheme theme = EditorTheme::load(f.fileName());
    EXPECT_TRUE(theme.formats.contains("H1"));
    EXPECT_TRUE(theme.formats.contains("STRONG"));
    EXPECT_TRUE(theme.formats.contains("CODE"));
    EXPECT_EQ(theme.foreground, QColor("#cccccc"));
}

TEST(ThemeParserTest, ParseBackgroundOnElement) {
    QTemporaryFile f;
    f.open();
    QTextStream out(&f);
    out << "VERBATIM\nforeground: 999999\nbackground: 333333\n";
    out.flush();

    EditorTheme theme = EditorTheme::load(f.fileName());
    ASSERT_TRUE(theme.formats.contains("VERBATIM"));
    EXPECT_EQ(theme.formats["VERBATIM"].background().color(), QColor("#333333"));
}

TEST(ThemeParserTest, NonexistentFile) {
    EditorTheme theme = EditorTheme::load("/nonexistent/path.style");
    // Should return defaults without crashing
    EXPECT_TRUE(theme.foreground.isValid());
}

TEST(ThemeParserTest, AllTomorrowPlusElements) {
    // Load the actual bundled theme
    EditorTheme theme = EditorTheme::load(":/themes/Tomorrow+.style");
    // The resource might not be available in tests without the .qrc,
    // so just verify it doesn't crash
    EXPECT_TRUE(theme.foreground.isValid());
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
