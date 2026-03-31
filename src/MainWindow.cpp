#include "MainWindow.h"
#include "Document.h"
#include "Editor.h"
#include "FindReplaceDialog.h"
#include "MarkdownRenderer.h"
#include "PreferencesDialog.h"
#include "PreviewWidget.h"
#include "Preferences.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPrinter>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
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
    m_splitter->setChildrenCollapsible(false);
    setCentralWidget(m_splitter);

    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    restoreWindowState();

    // Set initial 50/50 split if no saved state
    if (m_splitter->sizes().at(0) == 0 || m_splitter->sizes().at(1) == 0) {
        m_splitter->setSizes({600, 600});
    }

    // Welcome content for new documents
    if (m_document->isUntitled()) {
        QString welcome =
            "# Welcome to WinDown 3000\n\n"
            "Start typing Markdown here and see it rendered live on the right.\n\n"
            "## Features\n\n"
            "- **Bold**, *italic*, ~~strikethrough~~\n"
            "- [Links](https://github.com/andrewhofmann/WinDown3000)\n"
            "- Task lists: \n"
            "  - [x] Live preview\n"
            "  - [x] Syntax highlighting\n"
            "  - [ ] Your next great document\n\n"
            "## Code\n\n"
            "```python\nprint(\"Hello, WinDown 3000!\")\n```\n";
        m_editor->blockSignals(true);
        m_editor->setPlainText(welcome);
        m_editor->blockSignals(false);
        m_document->setMarkdown(welcome);
    }

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

    // Double-click in preview → navigate to source
    connect(m_preview, &PreviewWidget::textDoubleClicked,
            this, &MainWindow::onPreviewTextDoubleClicked);

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
        // Start at the top of the document
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cur);
        m_editor->verticalScrollBar()->setValue(0);
        m_editor->blockSignals(false);
        m_lastPreviewScroll = 0.0;
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

    // Ensure preview has the latest content before exporting
    m_document->renderNow();
    m_preview->printToPdf(path);
    connect(m_preview, &PreviewWidget::pdfExportFinished, this, [this, path](bool success) {
        if (success)
            statusBar()->showMessage(tr("PDF exported to %1").arg(path), 5000);
        else
            QMessageBox::warning(this, tr("Export Failed"),
                tr("Failed to export PDF."));
    }, Qt::SingleShotConnection);
}

void MainWindow::onFindReplace()
{
    if (!m_findReplace)
        m_findReplace = new FindReplaceDialog(m_editor, this);
    m_findReplace->showWithSelection();
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
    // Save preview scroll position before re-rendering, then restore after
    m_syncingScroll = true;
    m_preview->requestScrollPosition();
    m_preview->setHtml(html);

    // Restore scroll position after the new content loads
    // Use a short delay to let QWebEngine finish loading
    QTimer::singleShot(50, this, [this]() {
        m_preview->setScrollPosition(m_lastPreviewScroll);
        m_syncingScroll = false;
    });
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

    // Debounce: only sync after scrolling settles (avoids jitter during typing)
    if (!m_scrollSyncTimer) {
        m_scrollSyncTimer = new QTimer(this);
        m_scrollSyncTimer->setSingleShot(true);
        m_scrollSyncTimer->setInterval(30);
        connect(m_scrollSyncTimer, &QTimer::timeout, this, [this]() {
            if (m_syncingScroll) return;
            m_syncingScroll = true;
            QScrollBar *sb = m_editor->verticalScrollBar();
            if (sb->maximum() > 0) {
                qreal fraction = static_cast<qreal>(sb->value()) / sb->maximum();
                m_preview->setScrollPosition(fraction);
            }
            m_syncingScroll = false;
        });
    }
    m_scrollSyncTimer->start();
}

void MainWindow::onPreviewScrollChanged(qreal fraction)
{
    m_lastPreviewScroll = fraction;

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

    // Preserve both cursor position and scroll position
    m_editor->blockSignals(true);
    QTextCursor cur = m_editor->textCursor();
    int pos = cur.position();
    int scrollVal = m_editor->verticalScrollBar()->value();

    m_editor->setPlainText(updated);

    int charCount = m_editor->document()->characterCount();
    if (charCount > 0)
        cur.setPosition(qMin(pos, charCount - 1));
    else
        cur.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cur);
    m_editor->verticalScrollBar()->setValue(scrollVal);
    m_editor->blockSignals(false);

    m_document->setMarkdown(updated);
}

// ---------------------------------------------------------------------------
// Double-click in preview → find in source
// ---------------------------------------------------------------------------
void MainWindow::onPreviewTextDoubleClicked(const QString &data)
{
    // Data format: "searchText\x1Fposition" (unit separator between text and pos)
    int sep = data.indexOf(QChar(0x1F));
    QString text = (sep >= 0) ? data.left(sep) : data;
    qreal posFraction = (sep >= 0) ? data.mid(sep + 1).toDouble() : 0.0;

    if (text.isEmpty()) return;

    // Suppress scroll sync so the preview stays put
    m_syncingScroll = true;

    // Estimate where in the editor this text should be, based on the
    // click's vertical position in the preview
    int totalChars = m_editor->document()->characterCount();
    int estimatedPos = static_cast<int>(posFraction * totalChars);
    int searchStart = qMax(0, estimatedPos - totalChars / 10);

    QTextCursor cur = m_editor->textCursor();
    cur.setPosition(searchStart);
    m_editor->setTextCursor(cur);

    // Try exact text first, then progressively shorter substrings.
    // The rendered text won't have markdown syntax (** _ ` etc),
    // so shorter matches are more likely to succeed.
    bool found = false;
    QString searchText = text;

    // Try the full text
    found = m_editor->find(searchText);

    // If not found, try first line only (multi-line rendered text rarely
    // matches markdown source exactly)
    if (!found && searchText.contains('\n')) {
        searchText = searchText.section('\n', 0, 0).trimmed();
        cur.setPosition(searchStart);
        m_editor->setTextCursor(cur);
        found = m_editor->find(searchText);
    }

    // If still not found, try first few words (handles markdown syntax around text)
    if (!found && searchText.length() > 20) {
        QStringList words = searchText.split(' ', Qt::SkipEmptyParts);
        searchText = words.mid(0, qMin(3, words.size())).join(' ');
        cur.setPosition(searchStart);
        m_editor->setTextCursor(cur);
        found = m_editor->find(searchText);
    }

    // Last resort: search from the beginning
    if (!found) {
        cur.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cur);
        found = m_editor->find(searchText);
    }

    if (found) {
        m_editor->ensureCursorVisible();
        m_editor->centerCursor();
    } else {
        // If nothing found by text, at least scroll to the proportional position
        QScrollBar *sb = m_editor->verticalScrollBar();
        sb->setValue(static_cast<int>(posFraction * sb->maximum()));
    }

    m_editor->setFocus();
    QTimer::singleShot(150, this, [this]() { m_syncingScroll = false; });
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
    editMenu->addAction(tr("&Find and Replace..."), this, &MainWindow::onFindReplace,
                        QKeySequence::Find);

    // Format menu
    QMenu *formatMenu = menuBar()->addMenu(tr("F&ormat"));
    formatMenu->addAction(tr("&Bold"), m_editor, &Editor::toggleBold,
                          QKeySequence::Bold);
    formatMenu->addAction(tr("&Italic"), m_editor, &Editor::toggleItalic,
                          QKeySequence::Italic);
    formatMenu->addSeparator();

    // Header sub-menu
    QMenu *headerMenu = formatMenu->addMenu(tr("&Heading"));
    for (int i = 1; i <= 6; i++) {
        headerMenu->addAction(tr("H%1").arg(i), m_editor, [this, i]() {
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(QTextCursor::StartOfBlock);
            // Remove existing heading prefix
            QString line = cur.block().text();
            static QRegularExpression headRe(R"(^#{1,6}\s+)");
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
               "<p>A free Markdown viewer and editor.</p>"
               "<p>Developed and maintained by Andrew Hofmann.</p>"
               "<p>Based on MacDown 3000 by Schuyler Erle, "
               "which continues the legacy of Mou by Chen Luo "
               "and MacDown by Tzu-ping Chung.</p>"
               "<p><a href=\"https://github.com/andrewhofmann/WinDown3000\">"
               "github.com/andrewhofmann/WinDown3000</a></p>"
               "<p>Licensed under the MIT License.</p>")
            .arg(QApplication::applicationVersion()));
    });
}

void MainWindow::createToolBar()
{
    // Matches MacDown 3000's toolbar layout exactly:
    // [Shift Left|Shift Right] [Bold|Italic|Underline] [H1|H2|H3]
    //   [UL|OL]  Blockquote  Code  Link  Image  Copy HTML
    //   [Layout toggle]

    QToolBar *toolbar = addToolBar(tr("Formatting"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // Load toolbar icon — use raw black template images as-is.
    // On macOS the toolbar renders these with proper system contrast.
    // On Windows/dark themes, we invert to white.
    auto icon = [](const QString &name) {
        QPixmap src(QStringLiteral(":/icons/toolbar/%1.png").arg(name));
        if (src.isNull()) return QIcon();
        // Invert black template icons to white for visibility
        QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < img.height(); y++) {
            QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (int x = 0; x < img.width(); x++) {
                int a = qAlpha(line[x]);
                line[x] = qRgba(255, 255, 255, a);
            }
        }
        return QIcon(QPixmap::fromImage(img));
    };

    // -- Shift Left / Shift Right --
    toolbar->addAction(icon("shift-left"), tr("Shift Left"),
                       m_editor, &Editor::unindentSelection)
        ->setToolTip(tr("Shift Left"));

    toolbar->addAction(icon("shift-right"), tr("Shift Right"),
                       m_editor, &Editor::indentSelection)
        ->setToolTip(tr("Shift Right"));

    toolbar->addSeparator();

    // -- Text Styles: Bold, Italic, Underline --
    toolbar->addAction(icon("bold"), tr("Strong"), m_editor, &Editor::toggleBold)
        ->setToolTip(tr("Strong (Ctrl+B)"));

    toolbar->addAction(icon("italic"), tr("Emphasize"), m_editor, &Editor::toggleItalic)
        ->setToolTip(tr("Emphasize (Ctrl+I)"));

    toolbar->addAction(icon("underline"), tr("Underline"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        if (cur.hasSelection()) {
            QString sel = cur.selectedText();
            cur.insertText("_" + sel + "_");
        } else {
            cur.insertText("__");
            cur.movePosition(QTextCursor::Left);
            m_editor->setTextCursor(cur);
        }
    })->setToolTip(tr("Underline"));

    toolbar->addSeparator();

    // -- Headings: H1, H2, H3 --
    auto insertHeading = [this](int level) {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        QString line = cur.block().text();
        static QRegularExpression headRe(R"(^#{1,6}\s+)");
        auto match = headRe.match(line);
        if (match.hasMatch()) {
            cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
            cur.removeSelectedText();
        }
        cur.insertText(QString(level, '#') + " ");
    };

    toolbar->addAction(icon("h1"), tr("Heading 1"), [=]() { insertHeading(1); })
        ->setToolTip(tr("Heading 1 (Ctrl+1)"));
    toolbar->addAction(icon("h2"), tr("Heading 2"), [=]() { insertHeading(2); })
        ->setToolTip(tr("Heading 2 (Ctrl+2)"));
    toolbar->addAction(icon("h3"), tr("Heading 3"), [=]() { insertHeading(3); })
        ->setToolTip(tr("Heading 3 (Ctrl+3)"));

    toolbar->addSeparator();

    // -- Lists: Unordered, Ordered --
    toolbar->addAction(icon("ul"), tr("Unordered List"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("- ");
    })->setToolTip(tr("Unordered List"));

    toolbar->addAction(icon("ol"), tr("Ordered List"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("1. ");
    })->setToolTip(tr("Ordered List"));

    toolbar->addSeparator();

    // -- Block elements --
    toolbar->addAction(icon("blockquote"), tr("Blockquote"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::StartOfBlock);
        cur.insertText("> ");
    })->setToolTip(tr("Blockquote"));

    toolbar->addAction(icon("code"), tr("Inline Code"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        if (cur.hasSelection()) {
            QString sel = cur.selectedText();
            cur.insertText("`" + sel + "`");
        } else {
            cur.insertText("```\n\n```");
            cur.movePosition(QTextCursor::Up);
            m_editor->setTextCursor(cur);
        }
    })->setToolTip(tr("Inline Code"));

    toolbar->addAction(icon("link"), tr("Link"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        if (cur.hasSelection()) {
            QString sel = cur.selectedText();
            cur.insertText("[" + sel + "](url)");
        } else {
            cur.insertText("[text](url)");
        }
    })->setToolTip(tr("Link"));

    toolbar->addAction(icon("image"), tr("Image"), [this]() {
        QTextCursor cur = m_editor->textCursor();
        if (cur.hasSelection()) {
            QString sel = cur.selectedText();
            cur.insertText("![" + sel + "](url)");
        } else {
            cur.insertText("![alt](url)");
        }
    })->setToolTip(tr("Image"));

    toolbar->addSeparator();

    // -- Copy HTML --
    toolbar->addAction(icon("copyhtml"), tr("Copy HTML"), [this]() {
        QString html = m_document->renderer()->renderToHtml(m_document->markdown());
        QApplication::clipboard()->setText(html);
        statusBar()->showMessage(tr("HTML copied to clipboard"), 3000);
    })->setToolTip(tr("Copy HTML"));

    // -- Flexible space then Layout toggle --
    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Layout dropdown menu
    auto *layoutBtn = new QToolButton;
    layoutBtn->setIcon(icon("layout"));
    layoutBtn->setToolTip(tr("Layout"));
    layoutBtn->setPopupMode(QToolButton::InstantPopup);
    QMenu *layoutMenu = new QMenu(layoutBtn);
    layoutMenu->addAction(icon("hide-editor"), tr("Toggle Editor (Ctrl+E)"),
                          this, &MainWindow::onToggleEditor);
    layoutMenu->addAction(icon("hide-preview"), tr("Toggle Preview (Ctrl+P)"),
                          this, &MainWindow::onTogglePreview);
    layoutBtn->setMenu(layoutMenu);
    toolbar->addWidget(layoutBtn);
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
    PreferencesDialog dialog(m_prefs, this);
    dialog.exec();
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
