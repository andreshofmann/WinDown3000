#include "MarkdownRenderer.h"
#include "Preferences.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

// Hoedown C headers
#include "document.h"
#include "html.h"
#include "buffer.h"

MarkdownRenderer::MarkdownRenderer(Preferences *prefs, QObject *parent)
    : QObject(parent)
    , m_prefs(prefs)
{
}

// ---------------------------------------------------------------------------
// Hoedown extension / flag mapping
// ---------------------------------------------------------------------------
unsigned int MarkdownRenderer::hoedownExtensions() const
{
    unsigned int ext = 0;
    if (m_prefs->extensionTables())        ext |= HOEDOWN_EXT_TABLES;
    if (m_prefs->extensionFencedCode())    ext |= HOEDOWN_EXT_FENCED_CODE;
    if (m_prefs->extensionAutolink())      ext |= HOEDOWN_EXT_AUTOLINK;
    if (m_prefs->extensionStrikethrough()) ext |= HOEDOWN_EXT_STRIKETHROUGH;
    if (m_prefs->extensionUnderline())     ext |= HOEDOWN_EXT_UNDERLINE;
    if (m_prefs->extensionSuperscript())   ext |= HOEDOWN_EXT_SUPERSCRIPT;
    if (m_prefs->extensionHighlight())     ext |= HOEDOWN_EXT_HIGHLIGHT;
    if (m_prefs->extensionFootnotes())     ext |= HOEDOWN_EXT_FOOTNOTES;
    if (m_prefs->extensionSmartyPants())   ext |= HOEDOWN_EXT_NO_INTRA_EMPHASIS;
    if (m_prefs->extensionMath()) {
        ext |= HOEDOWN_EXT_MATH;
        if (m_prefs->htmlMathJaxInlineDollar())
            ext |= HOEDOWN_EXT_MATH_EXPLICIT;
    }
    return ext;
}

unsigned int MarkdownRenderer::hoedownHtmlFlags() const
{
    unsigned int flags = 0;
    if (m_prefs->htmlHardWrap()) flags |= HOEDOWN_HTML_HARD_WRAP;
    // Note: HOEDOWN_HTML_USE_TASK_LIST and HOEDOWN_HTML_BLOCKCODE_LINE_NUMBERS
    // are MacDown-specific patches not in upstream Hoedown 3.0.7.
    // Task list rendering is handled via post-processing instead.
    return flags;
}

// ---------------------------------------------------------------------------
// Front-matter stripping (Jekyll YAML)
// ---------------------------------------------------------------------------
QString MarkdownRenderer::stripFrontMatter(const QString &markdown) const
{
    if (!m_prefs->htmlDetectFrontMatter())
        return markdown;

    static QRegularExpression re(R"(\A---\s*\n.*?\n---\s*\n)",
                                  QRegularExpression::DotMatchesEverythingOption);
    QString result = markdown;
    result.replace(re, QString());
    return result;
}

// ---------------------------------------------------------------------------
// Core rendering: Markdown → HTML body
// ---------------------------------------------------------------------------
QString MarkdownRenderer::renderToHtml(const QString &markdown) const
{
    QString processed = stripFrontMatter(markdown);
    QByteArray utf8 = processed.toUtf8();

    hoedown_renderer *renderer = hoedown_html_renderer_new(
        static_cast<hoedown_html_flags>(hoedownHtmlFlags()), 0);
    if (!renderer) return QString();

    hoedown_document *doc = hoedown_document_new(
        renderer, static_cast<hoedown_extensions>(hoedownExtensions()), 16);
    if (!doc) {
        hoedown_html_renderer_free(renderer);
        return QString();
    }

    hoedown_buffer *buf = hoedown_buffer_new(utf8.size());
    if (!buf) {
        hoedown_document_free(doc);
        hoedown_html_renderer_free(renderer);
        return QString();
    }

    hoedown_document_render(doc, buf,
        reinterpret_cast<const uint8_t *>(utf8.constData()), utf8.size());

    QString html = QString::fromUtf8(
        reinterpret_cast<const char *>(buf->data), static_cast<int>(buf->size));

    hoedown_buffer_free(buf);
    hoedown_document_free(doc);
    hoedown_html_renderer_free(renderer);

    return html;
}

// ---------------------------------------------------------------------------
// Style tags for the preview page
// ---------------------------------------------------------------------------
QString MarkdownRenderer::buildStyleTags() const
{
    QString tags;

    // Preview CSS theme
    QString stylePath = QStringLiteral(":/styles/%1.css").arg(m_prefs->htmlStyleName());
    QFile styleFile(stylePath);
    if (styleFile.open(QIODevice::ReadOnly)) {
        tags += QStringLiteral("<style>\n%1\n</style>\n")
                    .arg(QString::fromUtf8(styleFile.readAll()));
    }

    // Prism syntax highlighting theme
    if (m_prefs->htmlSyntaxHighlighting()) {
        QString prismTheme = m_prefs->htmlHighlightingThemeName();
        QString prismPath = QStringLiteral(":/prism/themes/prism-%1.css").arg(prismTheme);
        if (prismTheme == "default")
            prismPath = QStringLiteral(":/prism/themes/prism.css");
        QFile prismFile(prismPath);
        if (prismFile.open(QIODevice::ReadOnly)) {
            tags += QStringLiteral("<style>\n%1\n</style>\n")
                        .arg(QString::fromUtf8(prismFile.readAll()));
        }
    }

    return tags;
}

// ---------------------------------------------------------------------------
// Script tags for the preview page
// ---------------------------------------------------------------------------
QString MarkdownRenderer::buildScriptTags() const
{
    QString tags;

    // Double-click in preview → send text + scroll fraction to editor
    tags += QStringLiteral(
        "<script>\n"
        "document.addEventListener('dblclick', function(e) {\n"
        "  var sel = window.getSelection().toString().trim();\n"
        "  if (!sel) {\n"
        "    var el = e.target.closest('h1,h2,h3,h4,h5,h6,p,li,td,th,blockquote,pre,code');\n"
        "    if (el) sel = el.textContent.trim().substring(0, 100);\n"
        "  }\n"
        "  if (sel) {\n"
        "    var rect = e.target.getBoundingClientRect();\n"
        "    var scrollH = document.documentElement.scrollHeight || 1;\n"
        "    var clickY = (document.documentElement.scrollTop + rect.top) / scrollH;\n"
        "    window.location.href = 'x-windown-navigate://find/' \n"
        "      + encodeURIComponent(sel) + '?pos=' + clickY.toFixed(4);\n"
        "  }\n"
        "});\n"
        "</script>\n");

    // Intercept external link clicks — allow in-document anchors (#id)
    tags += QStringLiteral(
        "<script>\n"
        "document.addEventListener('click', function(e) {\n"
        "  var a = e.target.closest('a');\n"
        "  if (a && a.href) {\n"
        "    var href = a.getAttribute('href');\n"
        "    if (href && href.startsWith('#')) return;\n"
        "    e.preventDefault();\n"
        "    e.stopPropagation();\n"
        "  }\n"
        "}, true);\n"
        "</script>\n");

    // Prism core + autoloader
    if (m_prefs->htmlSyntaxHighlighting()) {
        QFile prismCore(":/prism/prism.js");
        if (prismCore.open(QIODevice::ReadOnly)) {
            tags += QStringLiteral("<script>\n%1\n</script>\n")
                        .arg(QString::fromUtf8(prismCore.readAll()));
        }
    }

    // MathJax
    if (m_prefs->htmlMathJax()) {
        tags += QStringLiteral(
            "<script type=\"text/x-mathjax-config\">\n"
            "MathJax.Hub.Config({\n"
            "  tex2jax: { inlineMath: [['\\\\(','\\\\)']]%1 },\n"
            "  showProcessingMessages: false,\n"
            "  messageStyle: 'none'\n"
            "});\n"
            "</script>\n"
            "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.3/MathJax.js?"
            "config=TeX-AMS-MML_HTMLorMML\"></script>\n")
            .arg(m_prefs->htmlMathJaxInlineDollar()
                     ? QStringLiteral(", displayMath: [['$$','$$'],['\\\\[','\\\\]']]")
                     : QString());
    }

    // Mermaid
    if (m_prefs->htmlMermaid()) {
        tags += QStringLiteral(
            "<script src=\"https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js\"></script>\n"
            "<script>mermaid.initialize({startOnLoad:true, theme:'default'});</script>\n");
    }

    // Task list interactivity
    if (m_prefs->htmlTaskList()) {
        QFile taskJs(":/extensions/tasklist.js");
        if (taskJs.open(QIODevice::ReadOnly)) {
            tags += QStringLiteral("<script>\n%1\n</script>\n")
                        .arg(QString::fromUtf8(taskJs.readAll()));
        }
    }

    return tags;
}

// ---------------------------------------------------------------------------
// Full HTML page wrapping
// ---------------------------------------------------------------------------
QString MarkdownRenderer::wrapInTemplate(const QString &bodyHtml, const QString &title) const
{
    QString titleTag;
    if (!title.isEmpty())
        titleTag = QStringLiteral("<title>%1</title>").arg(title.toHtmlEscaped());

    return QStringLiteral(
        "<!DOCTYPE html>\n<html>\n<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "%1\n%2\n"
        "</head>\n<body>\n%3\n%4\n</body>\n</html>")
        .arg(titleTag, buildStyleTags(), bodyHtml, buildScriptTags());
}

QString MarkdownRenderer::renderFullPage(const QString &markdown, const QString &title) const
{
    return wrapInTemplate(renderToHtml(markdown), title);
}

QString MarkdownRenderer::renderForExport(const QString &markdown, const QString &title) const
{
    // For export, render without live-reload scripts but keep styles
    QString body = renderToHtml(markdown);
    QString titleTag;
    if (!title.isEmpty())
        titleTag = QStringLiteral("<title>%1</title>").arg(title.toHtmlEscaped());

    return QStringLiteral(
        "<!DOCTYPE html>\n<html>\n<head>\n"
        "<meta charset=\"utf-8\">\n"
        "%1\n%2\n"
        "</head>\n<body>\n%3\n</body>\n</html>")
        .arg(titleTag, buildStyleTags(), body);
}

// ---------------------------------------------------------------------------
// Toggle checkbox in source markdown
// ---------------------------------------------------------------------------
QString MarkdownRenderer::toggleCheckbox(const QString &markdown, int index)
{
    static QRegularExpression re(R"(^(\s*[-*+]\s+)\[([ xX])\])",
                                  QRegularExpression::MultilineOption);
    QString result = markdown;
    int count = 0;

    auto it = re.globalMatch(result);
    while (it.hasNext()) {
        auto match = it.next();
        if (count == index) {
            QString checkChar = match.captured(2);
            QString replacement = (checkChar == " ")
                ? match.captured(1) + "[x]"
                : match.captured(1) + "[ ]";
            result.replace(match.capturedStart(), match.capturedLength(), replacement);
            break;
        }
        count++;
    }

    return result;
}
