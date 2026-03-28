#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include "Preferences.h"

class PreferencesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear persisted settings so each test gets fresh defaults
        QSettings s("WinDown3000", "WinDown 3000");
        s.clear();
        s.sync();
        prefs = new Preferences();
    }
    void TearDown() override {
        delete prefs;
    }
    Preferences *prefs;
};

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------
TEST_F(PreferencesTest, DefaultEditorStyleName) {
    EXPECT_EQ(prefs->editorStyleName(), "Tomorrow+");
}

TEST_F(PreferencesTest, DefaultEditorFont) {
    QFont f = prefs->editorFont();
    EXPECT_EQ(f.pointSize(), 14);
}

TEST_F(PreferencesTest, DefaultHorizontalInset) {
    EXPECT_EQ(prefs->editorHorizontalInset(), 15);
}

TEST_F(PreferencesTest, DefaultVerticalInset) {
    EXPECT_EQ(prefs->editorVerticalInset(), 30);
}

TEST_F(PreferencesTest, DefaultLineSpacing) {
    EXPECT_DOUBLE_EQ(prefs->editorLineSpacing(), 3.0);
}

TEST_F(PreferencesTest, DefaultWidthLimited) {
    EXPECT_FALSE(prefs->editorWidthLimited());
}

TEST_F(PreferencesTest, DefaultMaximumWidth) {
    EXPECT_EQ(prefs->editorMaximumWidth(), 800);
}

TEST_F(PreferencesTest, DefaultEditorOnRight) {
    EXPECT_FALSE(prefs->editorOnRight());
}

TEST_F(PreferencesTest, DefaultShowWordCount) {
    EXPECT_TRUE(prefs->editorShowWordCount());
}

TEST_F(PreferencesTest, DefaultSyncScrolling) {
    EXPECT_TRUE(prefs->editorSyncScrolling());
}

TEST_F(PreferencesTest, DefaultCompleteMatching) {
    EXPECT_TRUE(prefs->editorCompleteMatchingCharacters());
}

TEST_F(PreferencesTest, DefaultAutoIncrementLists) {
    EXPECT_TRUE(prefs->editorAutoIncrementNumberedLists());
}

TEST_F(PreferencesTest, DefaultConvertTabs) {
    EXPECT_TRUE(prefs->editorConvertTabs());
}

TEST_F(PreferencesTest, DefaultEnsureNewline) {
    EXPECT_TRUE(prefs->editorEnsuresNewlineAtEndOfFile());
}

// ---------------------------------------------------------------------------
// Markdown extension defaults
// ---------------------------------------------------------------------------
TEST_F(PreferencesTest, DefaultTables) { EXPECT_TRUE(prefs->extensionTables()); }
TEST_F(PreferencesTest, DefaultFencedCode) { EXPECT_TRUE(prefs->extensionFencedCode()); }
TEST_F(PreferencesTest, DefaultAutolink) { EXPECT_TRUE(prefs->extensionAutolink()); }
TEST_F(PreferencesTest, DefaultStrikethrough) { EXPECT_TRUE(prefs->extensionStrikethrough()); }
TEST_F(PreferencesTest, DefaultUnderline) { EXPECT_FALSE(prefs->extensionUnderline()); }
TEST_F(PreferencesTest, DefaultSuperscript) { EXPECT_FALSE(prefs->extensionSuperscript()); }
TEST_F(PreferencesTest, DefaultHighlight) { EXPECT_FALSE(prefs->extensionHighlight()); }
TEST_F(PreferencesTest, DefaultFootnotes) { EXPECT_FALSE(prefs->extensionFootnotes()); }
TEST_F(PreferencesTest, DefaultMath) { EXPECT_TRUE(prefs->extensionMath()); }
TEST_F(PreferencesTest, DefaultSmartyPants) { EXPECT_FALSE(prefs->extensionSmartyPants()); }

// ---------------------------------------------------------------------------
// HTML/Preview defaults
// ---------------------------------------------------------------------------
TEST_F(PreferencesTest, DefaultHtmlStyleName) { EXPECT_EQ(prefs->htmlStyleName(), "GitHub2"); }
TEST_F(PreferencesTest, DefaultTaskList) { EXPECT_TRUE(prefs->htmlTaskList()); }
TEST_F(PreferencesTest, DefaultHardWrap) { EXPECT_FALSE(prefs->htmlHardWrap()); }
TEST_F(PreferencesTest, DefaultMathJax) { EXPECT_TRUE(prefs->htmlMathJax()); }
TEST_F(PreferencesTest, DefaultMathJaxInline) { EXPECT_FALSE(prefs->htmlMathJaxInlineDollar()); }
TEST_F(PreferencesTest, DefaultSyntaxHighlighting) { EXPECT_TRUE(prefs->htmlSyntaxHighlighting()); }
TEST_F(PreferencesTest, DefaultHighlightTheme) { EXPECT_EQ(prefs->htmlHighlightingThemeName(), "default"); }
TEST_F(PreferencesTest, DefaultLineNumbers) { EXPECT_FALSE(prefs->htmlLineNumbers()); }
TEST_F(PreferencesTest, DefaultMermaid) { EXPECT_FALSE(prefs->htmlMermaid()); }
TEST_F(PreferencesTest, DefaultGraphviz) { EXPECT_FALSE(prefs->htmlGraphviz()); }
TEST_F(PreferencesTest, DefaultFrontMatter) { EXPECT_FALSE(prefs->htmlDetectFrontMatter()); }
TEST_F(PreferencesTest, DefaultRenderTOC) { EXPECT_FALSE(prefs->htmlRendersTOC()); }

// ---------------------------------------------------------------------------
// Setters and persistence
// ---------------------------------------------------------------------------
TEST_F(PreferencesTest, SetEditorStyleName) {
    prefs->setEditorStyleName("Mou Night");
    EXPECT_EQ(prefs->editorStyleName(), "Mou Night");
}

TEST_F(PreferencesTest, SetEditorFont) {
    QFont f("Courier New", 18);
    prefs->setEditorFont(f);
    QFont loaded = prefs->editorFont();
    EXPECT_EQ(loaded.pointSize(), 18);
}

TEST_F(PreferencesTest, SetBoolPreferences) {
    prefs->setExtensionFootnotes(true);
    EXPECT_TRUE(prefs->extensionFootnotes());
    prefs->setExtensionFootnotes(false);
    EXPECT_FALSE(prefs->extensionFootnotes());
}

TEST_F(PreferencesTest, SetIntPreferences) {
    prefs->setEditorHorizontalInset(42);
    EXPECT_EQ(prefs->editorHorizontalInset(), 42);
}

// ---------------------------------------------------------------------------
// Signal emission
// ---------------------------------------------------------------------------
TEST_F(PreferencesTest, ChangedSignalEmitted) {
    QSignalSpy spy(prefs, &Preferences::changed);
    prefs->setEditorStyleName("Writer");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(PreferencesTest, ChangedSignalOnBoolChange) {
    QSignalSpy spy(prefs, &Preferences::changed);
    prefs->setHtmlMathJax(false);
    prefs->setHtmlMathJax(true);
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(PreferencesTest, ChangedSignalOnIntChange) {
    QSignalSpy spy(prefs, &Preferences::changed);
    prefs->setEditorMaximumWidth(1000);
    EXPECT_EQ(spy.count(), 1);
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
