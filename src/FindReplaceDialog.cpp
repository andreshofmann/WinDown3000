#include "FindReplaceDialog.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

FindReplaceDialog::FindReplaceDialog(QPlainTextEdit *editor, QWidget *parent)
    : QDialog(parent)
    , m_editor(editor)
{
    setWindowTitle(tr("Find and Replace"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(420);

    m_findField = new QLineEdit;
    m_findField->setPlaceholderText(tr("Find..."));
    m_replaceField = new QLineEdit;
    m_replaceField->setPlaceholderText(tr("Replace with..."));

    m_caseSensitive = new QCheckBox(tr("Match &case"));
    m_wholeWord = new QCheckBox(tr("&Whole word"));
    m_useRegex = new QCheckBox(tr("Re&gex"));

    m_findNextBtn = new QPushButton(tr("Find &Next"));
    m_findPrevBtn = new QPushButton(tr("Find &Previous"));
    m_replaceBtn = new QPushButton(tr("&Replace"));
    m_replaceAllBtn = new QPushButton(tr("Replace &All"));
    auto *closeBtn = new QPushButton(tr("Close"));

    m_findNextBtn->setDefault(true);
    m_statusLabel = new QLabel;

    // Layout
    auto *mainLayout = new QVBoxLayout(this);

    auto *fieldLayout = new QGridLayout;
    fieldLayout->addWidget(new QLabel(tr("Find:")), 0, 0);
    fieldLayout->addWidget(m_findField, 0, 1);
    fieldLayout->addWidget(new QLabel(tr("Replace:")), 1, 0);
    fieldLayout->addWidget(m_replaceField, 1, 1);
    mainLayout->addLayout(fieldLayout);

    auto *optionsLayout = new QHBoxLayout;
    optionsLayout->addWidget(m_caseSensitive);
    optionsLayout->addWidget(m_wholeWord);
    optionsLayout->addWidget(m_useRegex);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_findNextBtn);
    buttonLayout->addWidget(m_findPrevBtn);
    buttonLayout->addWidget(m_replaceBtn);
    buttonLayout->addWidget(m_replaceAllBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addWidget(m_statusLabel);

    // Connections
    connect(m_findNextBtn, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(m_findPrevBtn, &QPushButton::clicked, this, &FindReplaceDialog::findPrevious);
    connect(m_replaceBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceCurrent);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::hide);
    connect(m_findField, &QLineEdit::textChanged, this, &FindReplaceDialog::updateButtons);
    connect(m_findField, &QLineEdit::returnPressed, this, &FindReplaceDialog::findNext);

    updateButtons();
}

void FindReplaceDialog::showWithSelection()
{
    QTextCursor cur = m_editor->textCursor();
    if (cur.hasSelection())
        m_findField->setText(cur.selectedText());
    m_findField->selectAll();
    m_findField->setFocus();
    m_statusLabel->clear();
    show();
    raise();
    activateWindow();
}

QTextDocument::FindFlags FindReplaceDialog::buildFlags() const
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitive->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_wholeWord->isChecked())
        flags |= QTextDocument::FindWholeWords;
    return flags;
}

void FindReplaceDialog::findNext()
{
    QString term = m_findField->text();
    if (term.isEmpty()) return;

    bool found = false;
    if (m_useRegex->isChecked()) {
        QRegularExpression re(term);
        if (m_caseSensitive->isChecked())
            re.setPatternOptions(QRegularExpression::NoPatternOption);
        else
            re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        found = m_editor->find(re);
        if (!found) {
            // Wrap around
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(QTextCursor::Start);
            m_editor->setTextCursor(cur);
            found = m_editor->find(re);
        }
    } else {
        found = m_editor->find(term, buildFlags());
        if (!found) {
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(QTextCursor::Start);
            m_editor->setTextCursor(cur);
            found = m_editor->find(term, buildFlags());
        }
    }

    m_statusLabel->setText(found ? QString() : tr("No matches found."));
}

void FindReplaceDialog::findPrevious()
{
    QString term = m_findField->text();
    if (term.isEmpty()) return;

    QTextDocument::FindFlags flags = buildFlags() | QTextDocument::FindBackward;

    bool found = false;
    if (m_useRegex->isChecked()) {
        QRegularExpression re(term);
        if (!m_caseSensitive->isChecked())
            re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        found = m_editor->find(re, QTextDocument::FindBackward);
        if (!found) {
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(QTextCursor::End);
            m_editor->setTextCursor(cur);
            found = m_editor->find(re, QTextDocument::FindBackward);
        }
    } else {
        found = m_editor->find(term, flags);
        if (!found) {
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(QTextCursor::End);
            m_editor->setTextCursor(cur);
            found = m_editor->find(term, flags);
        }
    }

    m_statusLabel->setText(found ? QString() : tr("No matches found."));
}

void FindReplaceDialog::replaceCurrent()
{
    QTextCursor cur = m_editor->textCursor();
    if (!cur.hasSelection()) {
        findNext();
        return;
    }

    cur.insertText(m_replaceField->text());
    findNext();
}

void FindReplaceDialog::replaceAll()
{
    QString term = m_findField->text();
    QString replacement = m_replaceField->text();
    if (term.isEmpty()) return;

    int count = 0;

    if (m_useRegex->isChecked()) {
        QRegularExpression re(term);
        if (!m_caseSensitive->isChecked())
            re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

        QString text = m_editor->toPlainText();
        // Count matches before replacing
        auto it = re.globalMatch(text);
        while (it.hasNext()) { it.next(); count++; }

        if (count > 0) {
            QString newText = text;
            newText.replace(re, replacement);
            QTextCursor cur = m_editor->textCursor();
            cur.select(QTextCursor::Document);
            cur.insertText(newText);
        }
    } else {
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cur);

        while (m_editor->find(term, buildFlags())) {
            QTextCursor found = m_editor->textCursor();
            found.insertText(replacement);
            count++;
        }
    }

    m_statusLabel->setText(count > 0
        ? tr("Replaced %1 occurrence(s).").arg(count)
        : tr("No matches found."));
}

void FindReplaceDialog::updateButtons()
{
    bool hasText = !m_findField->text().isEmpty();
    m_findNextBtn->setEnabled(hasText);
    m_findPrevBtn->setEnabled(hasText);
    m_replaceBtn->setEnabled(hasText);
    m_replaceAllBtn->setEnabled(hasText);
}
