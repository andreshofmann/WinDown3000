#include "PreferencesDialog.h"
#include "Preferences.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(Preferences *prefs, QWidget *parent)
    : QDialog(parent)
    , m_prefs(prefs)
{
    setWindowTitle(tr("Preferences"));
    setMinimumSize(520, 480);

    auto *tabs = new QTabWidget;
    tabs->addTab(createEditorTab(),   tr("Editor"));
    tabs->addTab(createMarkdownTab(), tr("Markdown"));
    tabs->addTab(createPreviewTab(),  tr("Preview"));

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        applySettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &PreferencesDialog::applySettings);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);

    loadSettings();
}

// ---------------------------------------------------------------------------
// Editor tab
// ---------------------------------------------------------------------------
QWidget *PreferencesDialog::createEditorTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    // Font group
    auto *fontGroup = new QGroupBox(tr("Font"));
    auto *fontLayout = new QFormLayout(fontGroup);
    m_fontFamily = new QFontComboBox;
    m_fontFamily->setFontFilters(QFontComboBox::MonospacedFonts);
    m_fontSize = new QSpinBox;
    m_fontSize->setRange(6, 72);
    fontLayout->addRow(tr("Family:"), m_fontFamily);
    fontLayout->addRow(tr("Size:"), m_fontSize);
    layout->addWidget(fontGroup);

    // Theme
    auto *themeGroup = new QGroupBox(tr("Theme"));
    auto *themeLayout = new QFormLayout(themeGroup);
    m_editorTheme = new QComboBox;
    m_editorTheme->addItems({
        "Mou Fresh Air", "Mou Fresh Air+",
        "Mou Night", "Mou Night+",
        "Mou Paper", "Mou Paper+",
        "Solarized (Dark)", "Solarized (Dark)+",
        "Solarized (Light)", "Solarized (Light)+",
        "Tomorrow", "Tomorrow+", "Tomorrow Blue",
        "Writer", "Writer+"
    });
    themeLayout->addRow(tr("Editor theme:"), m_editorTheme);
    layout->addWidget(themeGroup);

    // Layout group
    auto *layoutGroup = new QGroupBox(tr("Layout"));
    auto *layoutForm = new QFormLayout(layoutGroup);
    m_horizontalInset = new QSpinBox;
    m_horizontalInset->setRange(0, 100);
    m_horizontalInset->setSuffix(tr(" px"));
    m_verticalInset = new QSpinBox;
    m_verticalInset->setRange(0, 100);
    m_verticalInset->setSuffix(tr(" px"));
    m_lineSpacing = new QDoubleSpinBox;
    m_lineSpacing->setRange(0.0, 20.0);
    m_lineSpacing->setSingleStep(0.5);
    m_widthLimited = new QCheckBox(tr("Limit editor width"));
    m_maxWidth = new QSpinBox;
    m_maxWidth->setRange(200, 2000);
    m_maxWidth->setSuffix(tr(" px"));
    m_editorOnRight = new QCheckBox(tr("Editor on right side"));

    layoutForm->addRow(tr("Horizontal inset:"), m_horizontalInset);
    layoutForm->addRow(tr("Vertical inset:"), m_verticalInset);
    layoutForm->addRow(tr("Line spacing:"), m_lineSpacing);
    layoutForm->addRow(m_widthLimited);
    layoutForm->addRow(tr("Maximum width:"), m_maxWidth);
    layoutForm->addRow(m_editorOnRight);
    layout->addWidget(layoutGroup);

    // Behavior group
    auto *behaviorGroup = new QGroupBox(tr("Behavior"));
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);
    m_showWordCount = new QCheckBox(tr("Show word count in status bar"));
    m_syncScrolling = new QCheckBox(tr("Sync editor and preview scrolling"));
    m_completeMatching = new QCheckBox(tr("Auto-complete matching brackets and quotes"));
    m_autoIncrementLists = new QCheckBox(tr("Auto-increment numbered lists"));
    m_convertTabs = new QCheckBox(tr("Convert tabs to spaces"));
    m_ensureNewline = new QCheckBox(tr("Ensure newline at end of file"));
    behaviorLayout->addWidget(m_showWordCount);
    behaviorLayout->addWidget(m_syncScrolling);
    behaviorLayout->addWidget(m_completeMatching);
    behaviorLayout->addWidget(m_autoIncrementLists);
    behaviorLayout->addWidget(m_convertTabs);
    behaviorLayout->addWidget(m_ensureNewline);
    layout->addWidget(behaviorGroup);

    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------------------
// Markdown tab
// ---------------------------------------------------------------------------
QWidget *PreferencesDialog::createMarkdownTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *group = new QGroupBox(tr("Markdown Extensions"));
    auto *groupLayout = new QVBoxLayout(group);

    m_extTables = new QCheckBox(tr("Tables (GFM)"));
    m_extFencedCode = new QCheckBox(tr("Fenced code blocks"));
    m_extAutolink = new QCheckBox(tr("Autolink URLs"));
    m_extStrikethrough = new QCheckBox(tr("Strikethrough (~~text~~)"));
    m_extUnderline = new QCheckBox(tr("Underline (_text_)"));
    m_extSuperscript = new QCheckBox(tr("Superscript (^text^)"));
    m_extHighlight = new QCheckBox(tr("Highlight (==text==)"));
    m_extFootnotes = new QCheckBox(tr("Footnotes ([^1])"));
    m_extMath = new QCheckBox(tr("Math notation ($$...$$)"));
    m_extSmartyPants = new QCheckBox(tr("SmartyPants (smart quotes and dashes)"));

    groupLayout->addWidget(m_extTables);
    groupLayout->addWidget(m_extFencedCode);
    groupLayout->addWidget(m_extAutolink);
    groupLayout->addWidget(m_extStrikethrough);
    groupLayout->addWidget(m_extUnderline);
    groupLayout->addWidget(m_extSuperscript);
    groupLayout->addWidget(m_extHighlight);
    groupLayout->addWidget(m_extFootnotes);
    groupLayout->addWidget(m_extMath);
    groupLayout->addWidget(m_extSmartyPants);

    layout->addWidget(group);
    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------------------
// Preview tab
// ---------------------------------------------------------------------------
QWidget *PreferencesDialog::createPreviewTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    // Theme
    auto *themeGroup = new QGroupBox(tr("Preview Theme"));
    auto *themeLayout = new QFormLayout(themeGroup);
    m_previewTheme = new QComboBox;
    m_previewTheme->addItems({
        "GitHub", "GitHub2", "GitHub-2020", "GitHub Tomorrow",
        "Clearness", "Clearness Dark",
        "Solarized (Light)", "Solarized (Dark)"
    });
    themeLayout->addRow(tr("CSS theme:"), m_previewTheme);
    layout->addWidget(themeGroup);

    // Rendering options
    auto *renderGroup = new QGroupBox(tr("Rendering"));
    auto *renderLayout = new QVBoxLayout(renderGroup);

    m_taskList = new QCheckBox(tr("Interactive task list checkboxes"));
    m_hardWrap = new QCheckBox(tr("Hard wrap (newlines become <br>)"));
    m_frontMatter = new QCheckBox(tr("Detect Jekyll front-matter"));
    m_renderTOC = new QCheckBox(tr("Render table of contents"));
    renderLayout->addWidget(m_taskList);
    renderLayout->addWidget(m_hardWrap);
    renderLayout->addWidget(m_frontMatter);
    renderLayout->addWidget(m_renderTOC);
    layout->addWidget(renderGroup);

    // Code highlighting
    auto *codeGroup = new QGroupBox(tr("Code Highlighting"));
    auto *codeLayout = new QFormLayout(codeGroup);
    m_syntaxHighlight = new QCheckBox(tr("Enable syntax highlighting in code blocks"));
    m_highlightTheme = new QComboBox;
    m_highlightTheme->addItems({
        "default", "dark", "coy", "funky", "okaidia",
        "solarizedlight", "tomorrow", "twilight"
    });
    m_lineNumbers = new QCheckBox(tr("Show line numbers in code blocks"));
    codeLayout->addRow(m_syntaxHighlight);
    codeLayout->addRow(tr("Prism theme:"), m_highlightTheme);
    codeLayout->addRow(m_lineNumbers);
    layout->addWidget(codeGroup);

    // Advanced rendering
    auto *advGroup = new QGroupBox(tr("Advanced"));
    auto *advLayout = new QVBoxLayout(advGroup);
    m_mathJax = new QCheckBox(tr("MathJax (math notation rendering)"));
    m_mathJaxInline = new QCheckBox(tr("  Allow inline $...$ math"));
    m_mermaid = new QCheckBox(tr("Mermaid (diagrams)"));
    m_graphviz = new QCheckBox(tr("Graphviz (dot diagrams)"));
    advLayout->addWidget(m_mathJax);
    advLayout->addWidget(m_mathJaxInline);
    advLayout->addWidget(m_mermaid);
    advLayout->addWidget(m_graphviz);
    layout->addWidget(advGroup);

    // Enable/disable inline dollar based on MathJax toggle
    connect(m_mathJax, &QCheckBox::toggled, m_mathJaxInline, &QCheckBox::setEnabled);

    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------------------
// Load / Apply
// ---------------------------------------------------------------------------
void PreferencesDialog::loadSettings()
{
    // Editor
    QFont f = m_prefs->editorFont();
    m_fontFamily->setCurrentFont(f);
    m_fontSize->setValue(f.pointSize());
    m_editorTheme->setCurrentText(m_prefs->editorStyleName());
    m_horizontalInset->setValue(m_prefs->editorHorizontalInset());
    m_verticalInset->setValue(m_prefs->editorVerticalInset());
    m_lineSpacing->setValue(m_prefs->editorLineSpacing());
    m_widthLimited->setChecked(m_prefs->editorWidthLimited());
    m_maxWidth->setValue(m_prefs->editorMaximumWidth());
    m_editorOnRight->setChecked(m_prefs->editorOnRight());
    m_showWordCount->setChecked(m_prefs->editorShowWordCount());
    m_syncScrolling->setChecked(m_prefs->editorSyncScrolling());
    m_completeMatching->setChecked(m_prefs->editorCompleteMatchingCharacters());
    m_autoIncrementLists->setChecked(m_prefs->editorAutoIncrementNumberedLists());
    m_convertTabs->setChecked(m_prefs->editorConvertTabs());
    m_ensureNewline->setChecked(m_prefs->editorEnsuresNewlineAtEndOfFile());

    // Markdown
    m_extTables->setChecked(m_prefs->extensionTables());
    m_extFencedCode->setChecked(m_prefs->extensionFencedCode());
    m_extAutolink->setChecked(m_prefs->extensionAutolink());
    m_extStrikethrough->setChecked(m_prefs->extensionStrikethrough());
    m_extUnderline->setChecked(m_prefs->extensionUnderline());
    m_extSuperscript->setChecked(m_prefs->extensionSuperscript());
    m_extHighlight->setChecked(m_prefs->extensionHighlight());
    m_extFootnotes->setChecked(m_prefs->extensionFootnotes());
    m_extMath->setChecked(m_prefs->extensionMath());
    m_extSmartyPants->setChecked(m_prefs->extensionSmartyPants());

    // Preview
    m_previewTheme->setCurrentText(m_prefs->htmlStyleName());
    m_taskList->setChecked(m_prefs->htmlTaskList());
    m_hardWrap->setChecked(m_prefs->htmlHardWrap());
    m_mathJax->setChecked(m_prefs->htmlMathJax());
    m_mathJaxInline->setChecked(m_prefs->htmlMathJaxInlineDollar());
    m_mathJaxInline->setEnabled(m_prefs->htmlMathJax());
    m_syntaxHighlight->setChecked(m_prefs->htmlSyntaxHighlighting());
    m_highlightTheme->setCurrentText(m_prefs->htmlHighlightingThemeName());
    m_lineNumbers->setChecked(m_prefs->htmlLineNumbers());
    m_mermaid->setChecked(m_prefs->htmlMermaid());
    m_graphviz->setChecked(m_prefs->htmlGraphviz());
    m_frontMatter->setChecked(m_prefs->htmlDetectFrontMatter());
    m_renderTOC->setChecked(m_prefs->htmlRendersTOC());
}

void PreferencesDialog::applySettings()
{
    // Editor
    QFont f = m_fontFamily->currentFont();
    f.setPointSize(m_fontSize->value());
    m_prefs->setEditorFont(f);
    m_prefs->setEditorStyleName(m_editorTheme->currentText());
    m_prefs->setEditorHorizontalInset(m_horizontalInset->value());
    m_prefs->setEditorVerticalInset(m_verticalInset->value());
    m_prefs->setEditorLineSpacing(m_lineSpacing->value());
    m_prefs->setEditorWidthLimited(m_widthLimited->isChecked());
    m_prefs->setEditorMaximumWidth(m_maxWidth->value());
    m_prefs->setEditorOnRight(m_editorOnRight->isChecked());
    m_prefs->setEditorShowWordCount(m_showWordCount->isChecked());
    m_prefs->setEditorSyncScrolling(m_syncScrolling->isChecked());
    m_prefs->setEditorCompleteMatchingCharacters(m_completeMatching->isChecked());
    m_prefs->setEditorAutoIncrementNumberedLists(m_autoIncrementLists->isChecked());
    m_prefs->setEditorConvertTabs(m_convertTabs->isChecked());
    m_prefs->setEditorEnsuresNewlineAtEndOfFile(m_ensureNewline->isChecked());

    // Markdown
    m_prefs->setExtensionTables(m_extTables->isChecked());
    m_prefs->setExtensionFencedCode(m_extFencedCode->isChecked());
    m_prefs->setExtensionAutolink(m_extAutolink->isChecked());
    m_prefs->setExtensionStrikethrough(m_extStrikethrough->isChecked());
    m_prefs->setExtensionUnderline(m_extUnderline->isChecked());
    m_prefs->setExtensionSuperscript(m_extSuperscript->isChecked());
    m_prefs->setExtensionHighlight(m_extHighlight->isChecked());
    m_prefs->setExtensionFootnotes(m_extFootnotes->isChecked());
    m_prefs->setExtensionMath(m_extMath->isChecked());
    m_prefs->setExtensionSmartyPants(m_extSmartyPants->isChecked());

    // Preview
    m_prefs->setHtmlStyleName(m_previewTheme->currentText());
    m_prefs->setHtmlTaskList(m_taskList->isChecked());
    m_prefs->setHtmlHardWrap(m_hardWrap->isChecked());
    m_prefs->setHtmlMathJax(m_mathJax->isChecked());
    m_prefs->setHtmlMathJaxInlineDollar(m_mathJaxInline->isChecked());
    m_prefs->setHtmlSyntaxHighlighting(m_syntaxHighlight->isChecked());
    m_prefs->setHtmlHighlightingThemeName(m_highlightTheme->currentText());
    m_prefs->setHtmlLineNumbers(m_lineNumbers->isChecked());
    m_prefs->setHtmlMermaid(m_mermaid->isChecked());
    m_prefs->setHtmlGraphviz(m_graphviz->isChecked());
    m_prefs->setHtmlDetectFrontMatter(m_frontMatter->isChecked());
    m_prefs->setHtmlRendersTOC(m_renderTOC->isChecked());
}
