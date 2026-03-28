#pragma once

#include <QObject>
#include <QString>

class Preferences;

/// Wraps the Hoedown C library to convert Markdown to HTML.
/// Mirrors the rendering pipeline of MacDown 3000's MPRenderer.
class MarkdownRenderer : public QObject
{
    Q_OBJECT

public:
    explicit MarkdownRenderer(Preferences *prefs, QObject *parent = nullptr);

    /// Convert raw Markdown text to an HTML body fragment.
    QString renderToHtml(const QString &markdown) const;

    /// Wrap the body HTML in the full page template (CSS, JS, etc.).
    QString wrapInTemplate(const QString &bodyHtml, const QString &title = QString()) const;

    /// Convenience: markdown → full HTML page ready for preview.
    QString renderFullPage(const QString &markdown, const QString &title = QString()) const;

    /// Get HTML suitable for export (no live-reload scripts).
    QString renderForExport(const QString &markdown, const QString &title = QString()) const;

    /// Toggle a task-list checkbox at the given index in the source markdown.
    /// Returns the modified markdown string.
    static QString toggleCheckbox(const QString &markdown, int index);

signals:
    void rendered(const QString &html);

private:
    QString buildStyleTags() const;
    QString buildScriptTags() const;
    unsigned int hoedownExtensions() const;
    unsigned int hoedownHtmlFlags() const;
    QString stripFrontMatter(const QString &markdown) const;

    Preferences *m_prefs;
};
