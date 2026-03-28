#pragma once

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QTimer>

class MarkdownRenderer;
class Preferences;

/// Document model — holds the markdown source, file path, and dirty state.
/// Coordinates rendering and file watching (mirrors MPDocument).
class Document : public QObject
{
    Q_OBJECT

public:
    explicit Document(Preferences *prefs, QObject *parent = nullptr);

    // File operations
    bool open(const QString &filePath);
    bool save();
    bool saveAs(const QString &filePath);
    bool isModified() const { return m_modified; }
    bool isUntitled() const { return m_filePath.isEmpty(); }
    QString filePath() const { return m_filePath; }
    QString fileName() const;
    QString title() const;

    // Content
    QString markdown() const { return m_markdown; }
    void setMarkdown(const QString &text);
    QString html() const { return m_html; }

    // Renderer
    MarkdownRenderer *renderer() const { return m_renderer; }

    /// Request a render (debounced — multiple calls within the interval
    /// are collapsed into one).
    void renderLater();

    /// Force an immediate synchronous render.
    void renderNow();

signals:
    void titleChanged(const QString &title);
    void htmlReady(const QString &html);
    void modifiedChanged(bool modified);
    void fileChangedExternally();

private:
    void onFileChanged(const QString &path);
    void setModified(bool modified);

    Preferences *m_prefs;
    MarkdownRenderer *m_renderer;
    QString m_filePath;
    QString m_markdown;
    QString m_html;
    bool m_modified = false;

    QFileSystemWatcher m_fileWatcher;
    QTimer m_renderTimer;
};
