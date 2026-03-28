#pragma once

#include <QMainWindow>

class Document;
class Editor;
class FindReplaceDialog;
class PreviewWidget;
class Preferences;

class QLabel;
class QSplitter;

/// Main application window with split-pane editor and preview.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // Setup
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void restoreWindowState();
    void saveWindowState();

    // Slots
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExportHtml();
    void onExportPdf();
    void onFindReplace();
    void onTextChanged();
    void onHtmlReady(const QString &html);
    void onPreferencesDialog();
    void onToggleEditor();
    void onTogglePreview();
    void onEditorScrollChanged();
    void onPreviewScrollChanged(qreal fraction);
    void onCheckboxToggled(int index);
    void onFileChangedExternally();
    void onWordCountChanged(int words, int chars);
    void updateTitle();
    bool maybeSave();

    // Components
    Preferences *m_prefs;
    Document *m_document;
    Editor *m_editor;
    PreviewWidget *m_preview;
    QSplitter *m_splitter;
    QLabel *m_wordCountLabel;
    FindReplaceDialog *m_findReplace = nullptr;

    // State
    bool m_syncingScroll = false;
};
