#include "Document.h"
#include "MarkdownRenderer.h"
#include "Preferences.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

Document::Document(Preferences *prefs, QObject *parent)
    : QObject(parent)
    , m_prefs(prefs)
    , m_renderer(new MarkdownRenderer(prefs, this))
{
    // Debounced render timer (50ms)
    m_renderTimer.setSingleShot(true);
    m_renderTimer.setInterval(50);
    connect(&m_renderTimer, &QTimer::timeout, this, &Document::renderNow);

    // File watcher for external changes
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &Document::onFileChanged);
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------
bool Document::open(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    m_markdown = in.readAll();
    m_filePath = filePath;
    setModified(false);

    // Watch for external changes
    if (!m_fileWatcher.files().isEmpty())
        m_fileWatcher.removePaths(m_fileWatcher.files());
    m_fileWatcher.addPath(filePath);

    renderNow();
    emit titleChanged(title());
    return true;
}

bool Document::save()
{
    if (m_filePath.isEmpty())
        return false;
    return saveAs(m_filePath);
}

bool Document::saveAs(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QString text = m_markdown;

    // Ensure trailing newline
    if (m_prefs->editorEnsuresNewlineAtEndOfFile() && !text.endsWith('\n'))
        text.append('\n');

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << text;

    // Update file watcher
    if (!m_fileWatcher.files().isEmpty())
        m_fileWatcher.removePaths(m_fileWatcher.files());

    m_filePath = filePath;
    m_fileWatcher.addPath(filePath);
    setModified(false);
    emit titleChanged(title());
    return true;
}

QString Document::fileName() const
{
    if (m_filePath.isEmpty())
        return tr("Untitled");
    return QFileInfo(m_filePath).fileName();
}

QString Document::title() const
{
    QString name = fileName();
    if (m_modified)
        name.prepend("* ");
    return name;
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------
void Document::setMarkdown(const QString &text)
{
    if (m_markdown == text)
        return;
    m_markdown = text;
    setModified(true);
    renderLater();
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
void Document::renderLater()
{
    m_renderTimer.start();
}

void Document::renderNow()
{
    m_renderTimer.stop();
    m_html = m_renderer->renderFullPage(m_markdown, fileName());
    emit htmlReady(m_html);
}

// ---------------------------------------------------------------------------
// File watching
// ---------------------------------------------------------------------------
void Document::onFileChanged(const QString &)
{
    emit fileChangedExternally();
}

void Document::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit modifiedChanged(modified);
}
