#include <gtest/gtest.h>
#include <QCoreApplication>
#include "MarkdownRenderer.h"
#include "Preferences.h"

class MarkdownRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        prefs = new Preferences();
        renderer = new MarkdownRenderer(prefs);
    }
    void TearDown() override {
        delete renderer;
        delete prefs;
    }
    Preferences *prefs;
    MarkdownRenderer *renderer;
};

// ---------------------------------------------------------------------------
// Basic rendering
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, EmptyInput) {
    EXPECT_EQ(renderer->renderToHtml(""), "");
}

TEST_F(MarkdownRendererTest, PlainParagraph) {
    QString html = renderer->renderToHtml("Hello world");
    EXPECT_TRUE(html.contains("<p>Hello world</p>"));
}

TEST_F(MarkdownRendererTest, MultipleParagraphs) {
    QString html = renderer->renderToHtml("First\n\nSecond");
    EXPECT_TRUE(html.contains("<p>First</p>"));
    EXPECT_TRUE(html.contains("<p>Second</p>"));
}

// ---------------------------------------------------------------------------
// Headings
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Heading1) {
    QString html = renderer->renderToHtml("# Heading 1");
    EXPECT_TRUE(html.contains("<h1>Heading 1</h1>"));
}

TEST_F(MarkdownRendererTest, Heading2) {
    QString html = renderer->renderToHtml("## Heading 2");
    EXPECT_TRUE(html.contains("<h2>Heading 2</h2>"));
}

TEST_F(MarkdownRendererTest, Heading3Through6) {
    for (int i = 3; i <= 6; i++) {
        QString md = QString(i, '#') + " Heading";
        QString html = renderer->renderToHtml(md);
        EXPECT_TRUE(html.contains(QStringLiteral("<h%1>Heading</h%1>").arg(i)))
            << "Failed for h" << i;
    }
}

// ---------------------------------------------------------------------------
// Inline formatting
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Bold) {
    QString html = renderer->renderToHtml("**bold**");
    EXPECT_TRUE(html.contains("<strong>bold</strong>"));
}

TEST_F(MarkdownRendererTest, Italic) {
    QString html = renderer->renderToHtml("*italic*");
    EXPECT_TRUE(html.contains("<em>italic</em>"));
}

TEST_F(MarkdownRendererTest, BoldItalic) {
    QString html = renderer->renderToHtml("***both***");
    EXPECT_TRUE(html.contains("<strong><em>both</em></strong>")
             || html.contains("<em><strong>both</strong></em>"));
}

TEST_F(MarkdownRendererTest, InlineCode) {
    QString html = renderer->renderToHtml("`code`");
    EXPECT_TRUE(html.contains("<code>code</code>"));
}

// ---------------------------------------------------------------------------
// Strikethrough (extension)
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Strikethrough) {
    prefs->setExtensionStrikethrough(true);
    renderer = new MarkdownRenderer(prefs);
    QString html = renderer->renderToHtml("~~deleted~~");
    EXPECT_TRUE(html.contains("<del>deleted</del>"));
}

// ---------------------------------------------------------------------------
// Links and images
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Link) {
    QString html = renderer->renderToHtml("[text](http://example.com)");
    EXPECT_TRUE(html.contains("<a href=\"http://example.com\">text</a>"));
}

TEST_F(MarkdownRendererTest, Image) {
    QString html = renderer->renderToHtml("![alt](image.png)");
    EXPECT_TRUE(html.contains("<img"));
    EXPECT_TRUE(html.contains("src=\"image.png\""));
    EXPECT_TRUE(html.contains("alt=\"alt\""));
}

// ---------------------------------------------------------------------------
// Lists
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, UnorderedList) {
    QString html = renderer->renderToHtml("- item 1\n- item 2");
    EXPECT_TRUE(html.contains("<ul>"));
    EXPECT_TRUE(html.contains("<li>item 1</li>"));
    EXPECT_TRUE(html.contains("<li>item 2</li>"));
}

TEST_F(MarkdownRendererTest, OrderedList) {
    QString html = renderer->renderToHtml("1. first\n2. second");
    EXPECT_TRUE(html.contains("<ol>"));
    EXPECT_TRUE(html.contains("<li>first</li>"));
    EXPECT_TRUE(html.contains("<li>second</li>"));
}

// ---------------------------------------------------------------------------
// Tables (extension)
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Table) {
    prefs->setExtensionTables(true);
    renderer = new MarkdownRenderer(prefs);
    QString md = "| A | B |\n|---|---|\n| 1 | 2 |";
    QString html = renderer->renderToHtml(md);
    EXPECT_TRUE(html.contains("<table>"));
    EXPECT_TRUE(html.contains("<th>A</th>"));
    EXPECT_TRUE(html.contains("<td>1</td>"));
}

// ---------------------------------------------------------------------------
// Fenced code blocks (extension)
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, FencedCodeBlock) {
    prefs->setExtensionFencedCode(true);
    renderer = new MarkdownRenderer(prefs);
    QString md = "```\ncode here\n```";
    QString html = renderer->renderToHtml(md);
    EXPECT_TRUE(html.contains("<code>"));
    EXPECT_TRUE(html.contains("code here"));
}

TEST_F(MarkdownRendererTest, FencedCodeBlockWithLanguage) {
    prefs->setExtensionFencedCode(true);
    renderer = new MarkdownRenderer(prefs);
    QString md = "```python\nprint('hi')\n```";
    QString html = renderer->renderToHtml(md);
    EXPECT_TRUE(html.contains("<code"));
    EXPECT_TRUE(html.contains("print("));
}

// ---------------------------------------------------------------------------
// Blockquotes
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Blockquote) {
    QString html = renderer->renderToHtml("> quoted text");
    EXPECT_TRUE(html.contains("<blockquote>"));
    EXPECT_TRUE(html.contains("quoted text"));
}

// ---------------------------------------------------------------------------
// Horizontal rules
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, HorizontalRule) {
    QString html = renderer->renderToHtml("---");
    EXPECT_TRUE(html.contains("<hr"));
}

// ---------------------------------------------------------------------------
// Math (extension)
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, MathBlock) {
    prefs->setExtensionMath(true);
    renderer = new MarkdownRenderer(prefs);
    QString md = "$$E = mc^2$$";
    QString html = renderer->renderToHtml(md);
    EXPECT_TRUE(html.contains("E = mc^2"));
}

// ---------------------------------------------------------------------------
// Footnotes (extension)
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Footnotes) {
    prefs->setExtensionFootnotes(true);
    renderer = new MarkdownRenderer(prefs);
    QString md = "Text[^1]\n\n[^1]: Footnote content";
    QString html = renderer->renderToHtml(md);
    EXPECT_TRUE(html.contains("Footnote content"));
}

// ---------------------------------------------------------------------------
// Autolink (extension)
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, Autolink) {
    prefs->setExtensionAutolink(true);
    renderer = new MarkdownRenderer(prefs);
    QString html = renderer->renderToHtml("Visit http://example.com today");
    EXPECT_TRUE(html.contains("<a href=\"http://example.com\">"));
}

// ---------------------------------------------------------------------------
// Front-matter stripping
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, FrontMatterStripped) {
    prefs->setHtmlDetectFrontMatter(true);
    renderer = new MarkdownRenderer(prefs);
    QString md = "---\ntitle: Test\n---\n# Hello";
    QString html = renderer->renderToHtml(md);
    EXPECT_FALSE(html.contains("title: Test"));
    EXPECT_TRUE(html.contains("<h1>Hello</h1>"));
}

TEST_F(MarkdownRendererTest, FrontMatterNotStrippedWhenDisabled) {
    prefs->setHtmlDetectFrontMatter(false);
    renderer = new MarkdownRenderer(prefs);
    QString md = "---\ntitle: Test\n---\n# Hello";
    QString html = renderer->renderToHtml(md);
    EXPECT_TRUE(html.contains("<h1>Hello</h1>"));
}

// ---------------------------------------------------------------------------
// Hard wrap
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, HardWrap) {
    prefs->setHtmlHardWrap(true);
    renderer = new MarkdownRenderer(prefs);
    QString html = renderer->renderToHtml("Line one\nLine two");
    EXPECT_TRUE(html.contains("<br"));
}

TEST_F(MarkdownRendererTest, NoHardWrap) {
    prefs->setHtmlHardWrap(false);
    renderer = new MarkdownRenderer(prefs);
    QString html = renderer->renderToHtml("Line one\nLine two");
    EXPECT_FALSE(html.contains("<br"));
}

// ---------------------------------------------------------------------------
// Checkbox toggle
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, ToggleCheckbox_UncheckedToChecked) {
    QString md = "- [ ] task one\n- [ ] task two";
    QString result = MarkdownRenderer::toggleCheckbox(md, 0);
    EXPECT_TRUE(result.startsWith("- [x] task one"));
    EXPECT_TRUE(result.contains("- [ ] task two"));
}

TEST_F(MarkdownRendererTest, ToggleCheckbox_CheckedToUnchecked) {
    QString md = "- [x] task one\n- [ ] task two";
    QString result = MarkdownRenderer::toggleCheckbox(md, 0);
    EXPECT_TRUE(result.startsWith("- [ ] task one"));
}

TEST_F(MarkdownRendererTest, ToggleCheckbox_SecondItem) {
    QString md = "- [x] first\n- [ ] second\n- [ ] third";
    QString result = MarkdownRenderer::toggleCheckbox(md, 1);
    EXPECT_TRUE(result.contains("- [x] first"));
    EXPECT_TRUE(result.contains("- [x] second"));
    EXPECT_TRUE(result.contains("- [ ] third"));
}

TEST_F(MarkdownRendererTest, ToggleCheckbox_OutOfRange) {
    QString md = "- [ ] only one";
    QString result = MarkdownRenderer::toggleCheckbox(md, 5);
    EXPECT_EQ(result, md); // unchanged
}

// ---------------------------------------------------------------------------
// Full page template
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, FullPageContainsDoctype) {
    QString html = renderer->renderFullPage("Hello");
    EXPECT_TRUE(html.startsWith("<!DOCTYPE html>"));
}

TEST_F(MarkdownRendererTest, FullPageContainsTitle) {
    QString html = renderer->renderFullPage("Hello", "My Doc");
    EXPECT_TRUE(html.contains("<title>My Doc</title>"));
}

TEST_F(MarkdownRendererTest, FullPageContainsBody) {
    QString html = renderer->renderFullPage("**bold**");
    EXPECT_TRUE(html.contains("<strong>bold</strong>"));
}

TEST_F(MarkdownRendererTest, ExportOmitsLiveScripts) {
    prefs->setHtmlMathJax(true);
    renderer = new MarkdownRenderer(prefs);
    QString exported = renderer->renderForExport("Hello");
    // Export should not contain MathJax CDN script
    EXPECT_FALSE(exported.contains("cdnjs.cloudflare.com"));
}

// ---------------------------------------------------------------------------
// Style / script tag generation
// ---------------------------------------------------------------------------
TEST_F(MarkdownRendererTest, MathJaxScriptsIncludedWhenEnabled) {
    prefs->setHtmlMathJax(true);
    renderer = new MarkdownRenderer(prefs);
    QString html = renderer->renderFullPage("math");
    EXPECT_TRUE(html.contains("MathJax"));
}

TEST_F(MarkdownRendererTest, MathJaxScriptsExcludedWhenDisabled) {
    prefs->setHtmlMathJax(false);
    renderer = new MarkdownRenderer(prefs);
    QString html = renderer->renderFullPage("math");
    EXPECT_FALSE(html.contains("MathJax"));
}

// Main for Qt event loop
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
