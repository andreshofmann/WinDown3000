#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTextStream>
#include "Document.h"
#include "Preferences.h"

class DocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        prefs = new Preferences();
        doc = new Document(prefs);
    }
    void TearDown() override {
        delete doc;
        delete prefs;
    }
    Preferences *prefs;
    Document *doc;

    QString createTempFile(const QString &content) {
        auto *f = new QTemporaryFile(this->doc);
        f->setAutoRemove(true);
        f->open();
        QTextStream out(f);
        out << content;
        out.flush();
        f->close();
        return f->fileName();
    }
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------
TEST_F(DocumentTest, InitiallyUntitled) {
    EXPECT_TRUE(doc->isUntitled());
}

TEST_F(DocumentTest, InitiallyNotModified) {
    EXPECT_FALSE(doc->isModified());
}

TEST_F(DocumentTest, InitiallyEmptyMarkdown) {
    EXPECT_TRUE(doc->markdown().isEmpty());
}

TEST_F(DocumentTest, DefaultFileName) {
    EXPECT_EQ(doc->fileName(), "Untitled");
}

// ---------------------------------------------------------------------------
// Set markdown
// ---------------------------------------------------------------------------
TEST_F(DocumentTest, SetMarkdownUpdatesContent) {
    doc->setMarkdown("# Hello");
    EXPECT_EQ(doc->markdown(), "# Hello");
}

TEST_F(DocumentTest, SetMarkdownSetsModified) {
    doc->setMarkdown("change");
    EXPECT_TRUE(doc->isModified());
}

TEST_F(DocumentTest, SetMarkdownEmitsModifiedChanged) {
    QSignalSpy spy(doc, &Document::modifiedChanged);
    doc->setMarkdown("change");
    EXPECT_GE(spy.count(), 1);
    EXPECT_TRUE(spy.last().at(0).toBool());
}

TEST_F(DocumentTest, SetSameMarkdownNoChange) {
    doc->setMarkdown("same");
    doc->setMarkdown("same"); // should not re-emit
    // Modified should still be true from first set
    EXPECT_TRUE(doc->isModified());
}

// ---------------------------------------------------------------------------
// Open file
// ---------------------------------------------------------------------------
TEST_F(DocumentTest, OpenFile) {
    QTemporaryFile f;
    f.open();
    QTextStream(&f) << "# Test Content";
    f.close();

    bool ok = doc->open(f.fileName());
    EXPECT_TRUE(ok);
    EXPECT_EQ(doc->markdown(), "# Test Content");
    EXPECT_FALSE(doc->isModified());
    EXPECT_FALSE(doc->isUntitled());
}

TEST_F(DocumentTest, OpenNonexistentFile) {
    bool ok = doc->open("/nonexistent/path.md");
    EXPECT_FALSE(ok);
}

TEST_F(DocumentTest, FileNameAfterOpen) {
    QTemporaryFile f;
    f.open();
    QTextStream(&f) << "content";
    f.close();

    doc->open(f.fileName());
    EXPECT_FALSE(doc->fileName().isEmpty());
    EXPECT_NE(doc->fileName(), "Untitled");
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------
TEST_F(DocumentTest, SaveUntitledReturnsFalse) {
    doc->setMarkdown("content");
    EXPECT_FALSE(doc->save());
}

TEST_F(DocumentTest, SaveAs) {
    doc->setMarkdown("# Saved Content");

    QTemporaryFile f;
    f.open();
    f.close();

    bool ok = doc->saveAs(f.fileName());
    EXPECT_TRUE(ok);
    EXPECT_FALSE(doc->isModified());

    // Verify file contents
    QFile readBack(f.fileName());
    readBack.open(QIODevice::ReadOnly);
    QString content = QTextStream(&readBack).readAll();
    EXPECT_TRUE(content.contains("# Saved Content"));
}

TEST_F(DocumentTest, SaveAfterSaveAs) {
    QTemporaryFile f;
    f.open();
    f.close();

    doc->setMarkdown("initial");
    doc->saveAs(f.fileName());
    doc->setMarkdown("updated");
    bool ok = doc->save();
    EXPECT_TRUE(ok);
    EXPECT_FALSE(doc->isModified());
}

TEST_F(DocumentTest, EnsureTrailingNewline) {
    prefs->setEditorEnsuresNewlineAtEndOfFile(true);

    QTemporaryFile f;
    f.open();
    f.close();

    doc->setMarkdown("no newline");
    doc->saveAs(f.fileName());

    QFile readBack(f.fileName());
    readBack.open(QIODevice::ReadOnly);
    QString content = QTextStream(&readBack).readAll();
    EXPECT_TRUE(content.endsWith('\n'));
}

// ---------------------------------------------------------------------------
// Title
// ---------------------------------------------------------------------------
TEST_F(DocumentTest, TitleContainsFileName) {
    QTemporaryFile f;
    f.open();
    QTextStream(&f) << "content";
    f.close();

    doc->open(f.fileName());
    EXPECT_FALSE(doc->title().isEmpty());
}

TEST_F(DocumentTest, TitleShowsModifiedIndicator) {
    QTemporaryFile f;
    f.open();
    QTextStream(&f) << "content";
    f.close();

    doc->open(f.fileName());
    doc->setMarkdown("changed");
    EXPECT_TRUE(doc->title().startsWith("* "));
}

TEST_F(DocumentTest, TitleChangedSignal) {
    QSignalSpy spy(doc, &Document::titleChanged);
    QTemporaryFile f;
    f.open();
    QTextStream(&f) << "content";
    f.close();

    doc->open(f.fileName());
    EXPECT_GE(spy.count(), 1);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
TEST_F(DocumentTest, RenderNowProducesHtml) {
    doc->setMarkdown("# Hello");
    doc->renderNow();
    EXPECT_FALSE(doc->html().isEmpty());
    EXPECT_TRUE(doc->html().contains("<h1>Hello</h1>"));
}

TEST_F(DocumentTest, HtmlReadySignal) {
    QSignalSpy spy(doc, &Document::htmlReady);
    doc->setMarkdown("test");
    doc->renderNow();
    EXPECT_GE(spy.count(), 1);
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
