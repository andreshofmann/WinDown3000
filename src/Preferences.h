#pragma once

#include <QSettings>
#include <QString>
#include <QFont>
#include <QObject>

class Preferences : public QObject
{
    Q_OBJECT

public:
    explicit Preferences(QObject *parent = nullptr);

    // -- Editor --
    QString editorStyleName() const;
    void setEditorStyleName(const QString &name);

    QFont editorFont() const;
    void setEditorFont(const QFont &font);

    int editorHorizontalInset() const;
    void setEditorHorizontalInset(int px);

    int editorVerticalInset() const;
    void setEditorVerticalInset(int px);

    qreal editorLineSpacing() const;
    void setEditorLineSpacing(qreal spacing);

    bool editorWidthLimited() const;
    void setEditorWidthLimited(bool limited);

    int editorMaximumWidth() const;
    void setEditorMaximumWidth(int px);

    bool editorOnRight() const;
    void setEditorOnRight(bool right);

    bool editorShowWordCount() const;
    void setEditorShowWordCount(bool show);

    bool editorSyncScrolling() const;
    void setEditorSyncScrolling(bool sync);

    bool editorCompleteMatchingCharacters() const;
    void setEditorCompleteMatchingCharacters(bool complete);

    bool editorAutoIncrementNumberedLists() const;
    void setEditorAutoIncrementNumberedLists(bool increment);

    bool editorConvertTabs() const;
    void setEditorConvertTabs(bool convert);

    bool editorEnsuresNewlineAtEndOfFile() const;
    void setEditorEnsuresNewlineAtEndOfFile(bool ensure);

    // -- Markdown extensions --
    bool extensionTables() const;
    void setExtensionTables(bool on);

    bool extensionFencedCode() const;
    void setExtensionFencedCode(bool on);

    bool extensionAutolink() const;
    void setExtensionAutolink(bool on);

    bool extensionStrikethrough() const;
    void setExtensionStrikethrough(bool on);

    bool extensionUnderline() const;
    void setExtensionUnderline(bool on);

    bool extensionSuperscript() const;
    void setExtensionSuperscript(bool on);

    bool extensionHighlight() const;
    void setExtensionHighlight(bool on);

    bool extensionFootnotes() const;
    void setExtensionFootnotes(bool on);

    bool extensionMath() const;
    void setExtensionMath(bool on);

    bool extensionSmartyPants() const;
    void setExtensionSmartyPants(bool on);

    // -- HTML / Preview rendering --
    QString htmlStyleName() const;
    void setHtmlStyleName(const QString &name);

    bool htmlTaskList() const;
    void setHtmlTaskList(bool on);

    bool htmlHardWrap() const;
    void setHtmlHardWrap(bool on);

    bool htmlMathJax() const;
    void setHtmlMathJax(bool on);

    bool htmlMathJaxInlineDollar() const;
    void setHtmlMathJaxInlineDollar(bool on);

    bool htmlSyntaxHighlighting() const;
    void setHtmlSyntaxHighlighting(bool on);

    QString htmlHighlightingThemeName() const;
    void setHtmlHighlightingThemeName(const QString &name);

    bool htmlLineNumbers() const;
    void setHtmlLineNumbers(bool on);

    bool htmlMermaid() const;
    void setHtmlMermaid(bool on);

    bool htmlGraphviz() const;
    void setHtmlGraphviz(bool on);

    bool htmlDetectFrontMatter() const;
    void setHtmlDetectFrontMatter(bool on);

    bool htmlRendersTOC() const;
    void setHtmlRendersTOC(bool on);

signals:
    void changed();

private:
    QSettings m_settings;
};
