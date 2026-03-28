#include "Preferences.h"

Preferences::Preferences(QObject *parent)
    : QObject(parent)
    , m_settings("WinDown3000", "WinDown 3000")
{
}

// ---------------------------------------------------------------------------
// Helper macros for boilerplate getters/setters
// ---------------------------------------------------------------------------
#define PREF_STRING(key, getter, setter, defaultVal) \
    QString Preferences::getter() const { return m_settings.value(key, defaultVal).toString(); } \
    void Preferences::setter(const QString &v) { m_settings.setValue(key, v); emit changed(); }

#define PREF_BOOL(key, getter, setter, defaultVal) \
    bool Preferences::getter() const { return m_settings.value(key, defaultVal).toBool(); } \
    void Preferences::setter(bool v) { m_settings.setValue(key, v); emit changed(); }

#define PREF_INT(key, getter, setter, defaultVal) \
    int Preferences::getter() const { return m_settings.value(key, defaultVal).toInt(); } \
    void Preferences::setter(int v) { m_settings.setValue(key, v); emit changed(); }

#define PREF_REAL(key, getter, setter, defaultVal) \
    qreal Preferences::getter() const { return m_settings.value(key, defaultVal).toReal(); } \
    void Preferences::setter(qreal v) { m_settings.setValue(key, v); emit changed(); }

// -- Editor ----------------------------------------------------------------
PREF_STRING("editor/styleName",   editorStyleName,   setEditorStyleName,   "Tomorrow+")

QFont Preferences::editorFont() const
{
#ifdef Q_OS_WIN
    QFont def("Consolas", 14);
#elif defined(Q_OS_MACOS)
    QFont def("Menlo", 14);
#else
    QFont def("Monospace", 14);
#endif
    def.setStyleHint(QFont::Monospace);
    QString family = m_settings.value("editor/fontFamily", def.family()).toString();
    int size = m_settings.value("editor/fontSize", def.pointSize()).toInt();
    QFont f(family, size);
    f.setStyleHint(QFont::Monospace);
    return f;
}

void Preferences::setEditorFont(const QFont &font)
{
    m_settings.setValue("editor/fontFamily", font.family());
    m_settings.setValue("editor/fontSize", font.pointSize());
    emit changed();
}

PREF_INT ("editor/horizontalInset",  editorHorizontalInset,  setEditorHorizontalInset,  15)
PREF_INT ("editor/verticalInset",    editorVerticalInset,    setEditorVerticalInset,    30)
PREF_REAL("editor/lineSpacing",      editorLineSpacing,      setEditorLineSpacing,      3.0)
PREF_BOOL("editor/widthLimited",     editorWidthLimited,     setEditorWidthLimited,     false)
PREF_INT ("editor/maximumWidth",     editorMaximumWidth,     setEditorMaximumWidth,     800)
PREF_BOOL("editor/onRight",          editorOnRight,          setEditorOnRight,          false)
PREF_BOOL("editor/showWordCount",    editorShowWordCount,    setEditorShowWordCount,    true)
PREF_BOOL("editor/syncScrolling",    editorSyncScrolling,    setEditorSyncScrolling,    true)
PREF_BOOL("editor/completeMatching", editorCompleteMatchingCharacters, setEditorCompleteMatchingCharacters, true)
PREF_BOOL("editor/autoIncrementLists", editorAutoIncrementNumberedLists, setEditorAutoIncrementNumberedLists, true)
PREF_BOOL("editor/convertTabs",      editorConvertTabs,      setEditorConvertTabs,      true)
PREF_BOOL("editor/ensureNewline",    editorEnsuresNewlineAtEndOfFile, setEditorEnsuresNewlineAtEndOfFile, true)

// -- Markdown extensions ---------------------------------------------------
PREF_BOOL("markdown/tables",         extensionTables,         setExtensionTables,         true)
PREF_BOOL("markdown/fencedCode",     extensionFencedCode,     setExtensionFencedCode,     true)
PREF_BOOL("markdown/autolink",       extensionAutolink,       setExtensionAutolink,       true)
PREF_BOOL("markdown/strikethrough",  extensionStrikethrough,  setExtensionStrikethrough,  true)
PREF_BOOL("markdown/underline",      extensionUnderline,      setExtensionUnderline,      false)
PREF_BOOL("markdown/superscript",    extensionSuperscript,    setExtensionSuperscript,    false)
PREF_BOOL("markdown/highlight",      extensionHighlight,      setExtensionHighlight,      false)
PREF_BOOL("markdown/footnotes",      extensionFootnotes,      setExtensionFootnotes,      false)
PREF_BOOL("markdown/math",           extensionMath,           setExtensionMath,           true)
PREF_BOOL("markdown/smartyPants",    extensionSmartyPants,    setExtensionSmartyPants,    false)

// -- HTML / Preview --------------------------------------------------------
PREF_STRING("html/styleName",        htmlStyleName,           setHtmlStyleName,           "GitHub2")
PREF_BOOL  ("html/taskList",         htmlTaskList,            setHtmlTaskList,            true)
PREF_BOOL  ("html/hardWrap",         htmlHardWrap,            setHtmlHardWrap,            false)
PREF_BOOL  ("html/mathJax",          htmlMathJax,             setHtmlMathJax,             true)
PREF_BOOL  ("html/mathJaxInline",    htmlMathJaxInlineDollar, setHtmlMathJaxInlineDollar, false)
PREF_BOOL  ("html/syntaxHighlight",  htmlSyntaxHighlighting,  setHtmlSyntaxHighlighting,  true)
PREF_STRING("html/highlightTheme",   htmlHighlightingThemeName, setHtmlHighlightingThemeName, "default")
PREF_BOOL  ("html/lineNumbers",      htmlLineNumbers,         setHtmlLineNumbers,         false)
PREF_BOOL  ("html/mermaid",          htmlMermaid,             setHtmlMermaid,             false)
PREF_BOOL  ("html/graphviz",         htmlGraphviz,            setHtmlGraphviz,            false)
PREF_BOOL  ("html/frontMatter",      htmlDetectFrontMatter,   setHtmlDetectFrontMatter,   false)
PREF_BOOL  ("html/renderTOC",        htmlRendersTOC,          setHtmlRendersTOC,          false)

#undef PREF_STRING
#undef PREF_BOOL
#undef PREF_INT
#undef PREF_REAL
