#include "MainWindow.h"
#include "Document.h"
#include "Editor.h"
#include "PreviewWidget.h"
#include "Preferences.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPrinter>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_prefs(new Preferences(this))
    , m_document(new Document(m_prefs, this))
    , m_editor(new Editor(m_prefs, this))
    , m_preview(new PreviewWidget(m_prefs, this))
    , m_splitter(new QSplitter(this))
    , m_wordCountLabel(new QLabel(this))
{
    setWindowTitle("WinDown 3000");
    setAcceptDrops(true);

    // Layout: editor | preview split
    if (m_prefs->editorOnRight()) {
        m_splitter->addWidget(m_preview);
        m_splitter->addWidget(m_editor);
    } else {
        m_splitter->addWidget(m_editor);
        m_splitter->addWidget(m_preview);
    }
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    setCentralWidget(m_splitter);

    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    restoreWindowState();

    // Wire up editor → document → preview
    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::onTextChanged);
    connect(m_document, &Document::htmlReady, this, &MainWindow::onHtmlReady);
    connect(m_document, &Document::titleChanged, this, &MainWindow::updateTitle);
    connect(m_document, &Document::fileChangedExternally, this, &MainWindow::onFileChangedExternally);
    connect(m_editor, &Editor::wordCountChanged, this, &MainWindow::onWordCountChanged);

    // Scroll sync
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::onEditorScrollChanged);
    connect(m_preview, &PreviewWidget::scrollPositionChanged,
            this, &MainWindow::onPreviewScrollChanged);

    // Checkbox toggle
    connect(m_preview, &PreviewWidget::checkboxToggled,
            this, &MainWindow::onCheckboxToggled);

    // Preferences changed
    connect(m_prefs, &Preferences::changed, this, [this]() {
        m_editor->applyPreferences();
        m_document->renderNow();
    });
}

MainWindow::~MainWindow()
{
    saveWindowState();
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------
void MainWindow::openFile(const QString &path)
{
    if (!maybeSave()) return;

    if (m_document->open(path)) {
        m_editor->blockSignals(true);
        m_editor->setPlainText(m_document->markdown());
        m_editor->blockSignals(false);
        updateTitle();
    }
}

void MainWindow::onNew()
{
    if (!maybeSave()) return;
    m_editor->blockSignals(true);
    m_editor->clear();
    m_editor->blockSignals(false);
    m_document->setMarkdown(QString());
    updateTitle();
}

void MainWindow::onOpen()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open Markdown File"), QString(),
        tr("Markdown Files (*.md *.markdown *.mdown *.mkd *.mkdn *.txt);;All Files (*)"));
    if (!path.isEmpty())
        openFile(path);
}

void MainWindow::onSave()
{
    if (m_document->isUntitled())
        onSaveAs();
    else
        m_document->save();
}

void MainWindow::onSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this,
        tr("Save Markdown File"), m_document->fileName(),
        tr("Markdown Files (*.md);;All Files (*)"));
    if (!path.isEmpty())
        m_document->saveAs(path);
}

void MainWindow::onExportHtml()
{
    QString path = QFileDialog::getSaveFileName(this,
        tr("Export HTML"), QString(),
        tr("HTML Files (*.html);;All Files (*)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << m_document->renderer()->renderForExport(
                   m_document->markdown(), m_document->fileName());
    }
}

void MainWindow::onExportPdf()
{
    QString path = QFileDialog::getSaveFileName(this,
        tr("Export PDF"), QString(),
        tr("PDF Files (*.pdf)"));
    if (path.isEmpty()) return;

    // PDF export via the preview's print-to-PDF
    // For QWebEngineView, use printToPdf; for WebView2, use PrintToPdf API
    m_preview->runJavaScript(QStringLiteral("window.print();"));
}

bool MainWindow::maybeSave()
{
    if (!m_document->isModified())
        return true;

    auto ret = QMessageBox::warning(this, tr("WinDown 3000"),
        tr("The document has been modified.\n"
           "Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        onSave();
        return !m_document->isModified();
    }
    return ret == QMessageBox::Discard;
}

// ---------------------------------------------------------------------------
// Editor ↔ Document ↔ Preview wiring
// ---------------------------------------------------------------------------
void MainWindow::onTextChanged()
{
    m_document->setMarkdown(m_editor->toPlainText());
}

void MainWindow::onHtmlReady(const QString &html)
{
    // Preserve scroll position across re-renders
    m_syncingScroll = true;
    m_preview->setHtml(html);
    m_syncingScroll = false;
}

void MainWindow::updateTitle()
{
    setWindowTitle(QStringLiteral("%1 — WinDown 3000").arg(m_document->title()));
}

// ---------------------------------------------------------------------------
// Scroll sync
// ---------------------------------------------------------------------------
void MainWindow::onEditorScrollChanged()
{
    if (m_syncingScroll || !m_prefs->editorSyncScrolling())
        return;

    m_syncingScroll = true;
    QScrollBar *sb = m_editor->verticalScrollBar();
    if (sb->maximum() > 0) {
        qreal fraction = static_cast<qreal>(sb->value()) / sb->maximum();
        m_preview->setScrollPosition(fraction);
    }
    m_syncingScroll = false;
}

void MainWindow::onPreviewScrollChanged(qreal fraction)
{
    if (m_syncingScroll || !m_prefs->editorSyncScrolling())
        return;

    m_syncingScroll = true;
    QScrollBar *sb = m_editor->verticalScrollBar();
    sb->setValue(static_cast<int>(fraction * sb->maximum()));
    m_syncingScroll = false;
}

// ---------------------------------------------------------------------------
// Checkbox toggle in preview → source
// ---------------------------------------------------------------------------
void MainWindow::onCheckboxToggled(int index)
{
    QString updated = MarkdownRenderer::toggleCheckbox(m_document->markdown(), index);
    m_editor->blockSignals(true);
    QTextCursor cur = m_editor->textCursor();
    int pos = cur.position();
    m_editor->setPlainText(updated);
    cur.setPosition(qMin(pos, m_editor->document()->characterCount() - 1));
    m_editor->setTextCursor(cur);
    m_editor->blockSignals(false);
    m_document->setMarkdown(updated);
}

// ---------------------------------------------------------------------------
// File watcher
// ---------------------------------------------------------------------------
void MainWindow::onFileChangedExternally()
{
    auto ret = QMessageBox::information(this, tr("File Changed"),
        tr("The file has been modified outside WinDown 3000.\n"
           "Do you want to reload it?"),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes)
        openFile(m_document->filePath());
}

// ---------------------------------------------------------------------------
// Word count
// ---------------------------------------------------------------------------
void MainWindow::onWordCountChanged(int words, int chars)
{
    m_wordCountLabel->setText(
        QStringLiteral("  %1 words  |  %2 characters  ").arg(words).arg(chars));
}

// ---------------------------------------------------------------------------
// Actions, Menus, Toolbar
// ---------------------------------------------------------------------------
void MainWindow::createActions() {}

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), this, &MainWindow::onNew,
                        QKeySequence::New);
    fileMenu->addAction(tr("&Open..."), this, &MainWindow::onOpen,
                        QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save"), this, &MainWindow::onSave,
                        QKeySequence::Save);
    fileMenu->addAction(tr("Save &As..."), this, &MainWindow::onSaveAs,
                        QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Export &HTML..."), this, &MainWindow::onExportHtml);
    fileMenu->addAction(tr("Export &PDF..."), this, &MainWindow::onExportPdf);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), qApp, &QApplication::quit,
                        QKeySequence::Quit);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Undo"), m_editor, &QPlainTextEdit::undo,
                        QKeySequence::Undo);
    editMenu->addAction(tr("&Redo"), m_editor, &QPlainTextEdit::redo,
                        QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction(tr("Cu&t"), m_editor, &QPlainTextEdit::cut,
                        QKeySequence::Cut);
    editMenu->addAction(tr("&Copy"), m_editor, &QPlainTextEdit::copy,
                        QKeySequence::Copy);
    editMenu->addAction(tr("&Paste"), m_editor, &QPlainTextEdit::paste,
                        QKeySequence::Paste);
    editMenu->addAction(tr("Select &All"), m_editor, &QPlainTextEdit::selectAll,
                        QKeySequence::SelectAll);
    editMenu->addSeparator();
    editMenu->addAction(tr("&Find..."), m_editor, [this]() {
        // TODO: find/replace dialog
    }, QKeySequence::Find);

    // Format menu
    QMenu *formatMenu = menuBar()->addMenu(tr("F&ormat"));
    formatMenu->addAction(tr("&Bold"), m_editor, [this]() {
        m_editor->keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_B, Qt::ControlModifier));
    }, QKeySequence::Bold);
    formatMenu->addAction(tr("&Italic"), m_editor, [this]() {
        m_editor->keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_I, Qt::ControlModifier));
    }, QKeySequence::Italic);
    formatMenu->addSeparator();

    // Header sub-menu
    QMenu *headerMenu = formatMenu->addMenu(tr("&Heading"));
    for (int i = 1; i <= 6; i++) {
        headerMenu->addAction(tr("H%1").arg(i), m_editor, [this, i]() {
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(QTextCursor::StartOfBlock);
            // Remove existing heading prefix
            QString line = cur.block().text();
            static QRegularExpression headRe(R"(^#{1,6}\s*)");
            auto match = headRe.match(line);
            if (match.hasMatch()) {
                cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cur.removeSelectedText();
            }
            cur.insertText(QString(i, '#') + " ");
        }, QKeySequence(QStringLiteral("Ctrl+%1").arg(i)));
    }

    formatMenu->addSeparator();
    formatMenu->addAction(tr("Insert &Link"), m_editor, [this]() {
        m_editor->textCursor().insertText("[text](url)");
    }, QKeySequence(tr("Ctrl+L")));
    formatMenu->addAction(tr("Insert I&mage"), m_editor, [this]() {
        m_editor->textCursor().insertText("![alt](url)");
    }, QKeySequence(tr("Ctrl+Shift+I")));
    formatMenu->addAction(tr("Insert &Code Block"), m_editor, [this]() {
        m_editor->textCursor().insertText("```\n\n```");
    });

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Toggle &Editor"), this, &MainWindow::onToggleEditor,
                        QKeySequence(tr("Ctrl+E")));
    viewMenu->addAction(tr("Toggle &Preview"), this, &MainWindow::onTogglePreview,
                        QKeySequence(tr("Ctrl+P")));
    viewMenu->addSeparator();
    viewMenu->addAction(tr("&Preferences..."), this, &MainWindow::onPreferencesDialog,
                        QKeySequence::Preferences);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About WinDown 3000"), this, [this]() {
        QMessageBox::about(this, tr("About WinDown 3000"),
            tr("<h2>WinDown 3000</h2>"
               "<p>Version %1</p>"
               "<p>A free Markdown editor for Windows.</p>"
               "<p>Based on MacDown 3000 by Schuyler Erle, "
               "which continues the legacy of Mou and MacDown.</p>"
               "<p>Licensed under the MIT License.</p>")
            .arg(QApplication::applicationVersion()));
    });
}

void MainWindow::createToolBar()
{
    QToolBar *toolbar = addToolBar(tr("Formatting"));
    toolbar->setMovable(false);

    toolbar->addAction(tr("B"), [this]() {
        m_editor->keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_B, Qt::ControlModifier));
    })->setToolTip(tr("Bold (Ctrl+B)"));

    toolbar->addAction(tr("I"), [this]() {
        m_editor->keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_I, Qt::ControlModifier));
    })->setToolTip(tr("Italic (Ctrl+I)"));

    toolbar->addSeparator();

    toolbar->addAction(tr("H1"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("# ");
    });

    toolbar->addAction(tr("H2"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("## ");
    });

    toolbar->addAction(tr("H3"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("### ");
    });

    toolbar->addSeparator();

    toolbar->addAction(tr("UL"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("- ");
    })->setToolTip(tr("Unordered List"));

    toolbar->addAction(tr("OL"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("1. ");
    })->setToolTip(tr("Ordered List"));

    toolbar->addAction(tr(">"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("> ");
    })->setToolTip(tr("Blockquote"));

    toolbar->addSeparator();

    toolbar->addAction(tr("Link"), [this]() {
        m_editor->textCursor().insertText("[text](url)");
    })->setToolTip(tr("Insert Link (Ctrl+L)"));

    toolbar->addAction(tr("Image"), [this]() {
        m_editor->textCursor().insertText("![alt](url)");
    })->setToolTip(tr("Insert Image"));

    toolbar->addAction(tr("Code"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        if (cur.hasSelection()) {
            // Inline code
            int start = cur.selectionStart();
            int end = cur.selectionEnd();
            QString sel = cur.selectedText();
            cur.insertText("`" + sel + "`");
        } else {
            cur.insertText("```\n\n```");
            cur.movePosition(QTextCursor::Up);
            m_editor->setTextCursor(cur);
        }
    })->setToolTip(tr("Code Block"));
}

void MainWindow::createStatusBar()
{
    m_wordCountLabel->setText(tr("  0 words  |  0 characters  "));
    statusBar()->addPermanentWidget(m_wordCountLabel);
    if (!m_prefs->editorShowWordCount())
        statusBar()->hide();
}

// ---------------------------------------------------------------------------
// View toggles
// ---------------------------------------------------------------------------
void MainWindow::onToggleEditor()
{
    m_editor->setVisible(!m_editor->isVisible());
}

void MainWindow::onTogglePreview()
{
    m_preview->setVisible(!m_preview->isVisible());
}

// ---------------------------------------------------------------------------
// Preferences dialog (stub — TODO: full dialog)
// ---------------------------------------------------------------------------
void MainWindow::onPreferencesDialog()
{
    // TODO: full preferences dialog
    QMessageBox::information(this, tr("Preferences"),
        tr("Preferences dialog coming soon.\n"
           "Settings are stored in the system registry."));
}

// ---------------------------------------------------------------------------
// Window state persistence
// ---------------------------------------------------------------------------
void MainWindow::restoreWindowState()
{
    QSettings s("WinDown3000", "WinDown 3000");
    restoreGeometry(s.value("window/geometry").toByteArray());
    restoreState(s.value("window/state").toByteArray());
    m_splitter->restoreState(s.value("window/splitter").toByteArray());
}

void MainWindow::saveWindowState()
{
    QSettings s("WinDown3000", "WinDown 3000");
    s.setValue("window/geometry", saveGeometry());
    s.setValue("window/state", saveState());
    s.setValue("window/splitter", m_splitter->saveState());
}

// ---------------------------------------------------------------------------
// Close / drag-drop
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        saveWindowState();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty())
        openFile(urls.first().toLocalFile());
}
