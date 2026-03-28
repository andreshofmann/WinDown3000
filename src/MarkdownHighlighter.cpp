#include "MarkdownHighlighter.h"
#include "Preferences.h"

MarkdownHighlighter::MarkdownHighlighter(Preferences *prefs, QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_prefs(prefs)
    , m_fenceOpen(R"(^```\w*\s*$)")
    , m_fenceClose(R"(^```\s*$)")
{
    buildRules();
}

void MarkdownHighlighter::setTheme(const EditorTheme &theme)
{
    m_theme = theme;
    rehighlight();
}

void MarkdownHighlighter::buildRules()
{
    m_rules.clear();

    // Headings (ATX style)
    m_rules.append({QRegularExpression(R"(^#{1}\s.+)"),  "H1"});
    m_rules.append({QRegularExpression(R"(^#{2}\s.+)"),  "H2"});
    m_rules.append({QRegularExpression(R"(^#{3}\s.+)"),  "H3"});
    m_rules.append({QRegularExpression(R"(^#{4}\s.+)"),  "H4"});
    m_rules.append({QRegularExpression(R"(^#{5}\s.+)"),  "H5"});
    m_rules.append({QRegularExpression(R"(^#{6}\s.+)"),  "H6"});

    // Horizontal rule
    m_rules.append({QRegularExpression(R"(^[-*_]{3,}\s*$)"), "HRULE"});

    // Blockquote
    m_rules.append({QRegularExpression(R"(^\s*>.+)"), "BLOCKQUOTE"});

    // Unordered list bullets
    m_rules.append({QRegularExpression(R"(^\s*[-*+]\s)"), "LIST_BULLET"});

    // Ordered list enumerators
    m_rules.append({QRegularExpression(R"(^\s*\d+\.\s)"), "LIST_ENUMERATOR"});

    // Bold / strong: **text** or __text__
    m_rules.append({QRegularExpression(R"(\*\*[^\*]+\*\*)"), "STRONG"});
    m_rules.append({QRegularExpression(R"(__[^_]+__)"),       "STRONG"});

    // Italic / emphasis: *text* or _text_
    m_rules.append({QRegularExpression(R"((?<!\*)\*(?!\*)[^\*]+\*(?!\*))"), "EMPH"});
    m_rules.append({QRegularExpression(R"((?<!_)_(?!_)[^_]+_(?!_))"),       "EMPH"});

    // Strikethrough: ~~text~~
    m_rules.append({QRegularExpression(R"(~~[^~]+~~)"), "EMPH"});

    // Inline code: `code`
    m_rules.append({QRegularExpression(R"(`[^`]+`)"), "CODE"});

    // Links: [text](url)
    m_rules.append({QRegularExpression(R"(\[[^\]]+\]\([^\)]+\))"), "LINK"});

    // Reference links: [text][ref]
    m_rules.append({QRegularExpression(R"(\[[^\]]+\]\[[^\]]*\])"), "REFERENCE"});

    // Auto-links: <http://...>
    m_rules.append({QRegularExpression(R"(<https?://[^>]+>)"), "AUTO_LINK_URL"});

    // Auto-link emails: <email@example.com>
    m_rules.append({QRegularExpression(R"(<[^@>]+@[^>]+>)"), "AUTO_LINK_EMAIL"});

    // Images: ![alt](url)
    m_rules.append({QRegularExpression(R"(!\[[^\]]*\]\([^\)]+\))"), "IMAGE"});

    // HTML entities: &amp; etc.
    m_rules.append({QRegularExpression(R"(&\w+;)"), "HTML_ENTITY"});

    // HTML comments
    m_rules.append({QRegularExpression(R"(<!--.*-->)"), "COMMENT"});
}

void MarkdownHighlighter::applyRule(const QString &text, const Rule &rule)
{
    auto it = m_theme.formats.constFind(rule.formatKey);
    if (it == m_theme.formats.constEnd())
        return;

    QRegularExpressionMatchIterator matchIt = rule.pattern.globalMatch(text);
    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), *it);
    }
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    // Apply single-line rules
    for (const Rule &rule : m_rules)
        applyRule(text, rule);

    // Multi-line fenced code block handling
    // State: 0 = normal, 1 = inside fenced code block
    int prevState = previousBlockState();
    if (prevState == -1) prevState = 0;

    if (prevState == 1) {
        // We're inside a code block
        auto it = m_theme.formats.constFind("VERBATIM");
        if (it != m_theme.formats.constEnd())
            setFormat(0, text.length(), *it);

        if (m_fenceClose.match(text).hasMatch())
            setCurrentBlockState(0);
        else
            setCurrentBlockState(1);
    } else {
        if (m_fenceOpen.match(text).hasMatch()) {
            auto it = m_theme.formats.constFind("VERBATIM");
            if (it != m_theme.formats.constEnd())
                setFormat(0, text.length(), *it);
            setCurrentBlockState(1);
        } else {
            setCurrentBlockState(0);
        }
    }
}
