#pragma once

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

/// Find and Replace dialog for the Markdown editor.
class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(QPlainTextEdit *editor, QWidget *parent = nullptr);

    /// Open the dialog and optionally populate with selected text.
    void showWithSelection();

private:
    void findNext();
    void findPrevious();
    void replaceCurrent();
    void replaceAll();
    void updateButtons();
    QTextDocument::FindFlags buildFlags() const;

    QPlainTextEdit *m_editor;
    QLineEdit *m_findField;
    QLineEdit *m_replaceField;
    QCheckBox *m_caseSensitive;
    QCheckBox *m_wholeWord;
    QCheckBox *m_useRegex;
    QPushButton *m_findNextBtn;
    QPushButton *m_findPrevBtn;
    QPushButton *m_replaceBtn;
    QPushButton *m_replaceAllBtn;
    QLabel *m_statusLabel;
};
