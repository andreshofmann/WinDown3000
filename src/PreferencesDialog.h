#pragma once

#include <QDialog>

class Preferences;

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QTabWidget;

/// Full preferences dialog with tabs for Editor, Markdown, and Preview settings.
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(Preferences *prefs, QWidget *parent = nullptr);

private:
    QWidget *createEditorTab();
    QWidget *createMarkdownTab();
    QWidget *createPreviewTab();
    void loadSettings();
    void applySettings();

    Preferences *m_prefs;

    // Editor tab
    QFontComboBox *m_fontFamily;
    QSpinBox *m_fontSize;
    QComboBox *m_editorTheme;
    QSpinBox *m_horizontalInset;
    QSpinBox *m_verticalInset;
    QDoubleSpinBox *m_lineSpacing;
    QCheckBox *m_widthLimited;
    QSpinBox *m_maxWidth;
    QCheckBox *m_editorOnRight;
    QCheckBox *m_showWordCount;
    QCheckBox *m_syncScrolling;
    QCheckBox *m_completeMatching;
    QCheckBox *m_autoIncrementLists;
    QCheckBox *m_convertTabs;
    QCheckBox *m_ensureNewline;

    // Markdown tab
    QCheckBox *m_extTables;
    QCheckBox *m_extFencedCode;
    QCheckBox *m_extAutolink;
    QCheckBox *m_extStrikethrough;
    QCheckBox *m_extUnderline;
    QCheckBox *m_extSuperscript;
    QCheckBox *m_extHighlight;
    QCheckBox *m_extFootnotes;
    QCheckBox *m_extMath;
    QCheckBox *m_extSmartyPants;

    // Preview tab
    QComboBox *m_previewTheme;
    QCheckBox *m_taskList;
    QCheckBox *m_hardWrap;
    QCheckBox *m_mathJax;
    QCheckBox *m_mathJaxInline;
    QCheckBox *m_syntaxHighlight;
    QComboBox *m_highlightTheme;
    QCheckBox *m_lineNumbers;
    QCheckBox *m_mermaid;
    QCheckBox *m_graphviz;
    QCheckBox *m_frontMatter;
    QCheckBox *m_renderTOC;
};
