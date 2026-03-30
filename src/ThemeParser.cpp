#include "ThemeParser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

EditorTheme EditorTheme::load(const QString &path)
{
    EditorTheme theme;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return theme;

    QTextStream in(&file);
    QString currentBlock;

    auto parseColor = [](const QString &hex) -> QColor {
        QColor c(QStringLiteral("#%1").arg(hex.trimmed()));
        if (!c.isValid())
            return QColor(0, 0, 0); // fallback to black for malformed values
        return c;
    };

    QTextCharFormat currentFormat;
    bool inElement = false;

    while (!in.atEnd()) {
        QString line = in.readLine();

        // Blank line or new block header
        if (line.trimmed().isEmpty()) {
            if (inElement && !currentBlock.isEmpty()) {
                if (currentBlock == "editor") {
                    // already handled inline
                } else if (currentBlock == "editor-selection") {
                    // already handled inline
                } else {
                    theme.formats[currentBlock] = currentFormat;
                }
            }
            currentBlock.clear();
            currentFormat = QTextCharFormat();
            inElement = false;
            continue;
        }

        // Property line (starts with whitespace or contains ':')
        if (line.contains(':')) {
            QString key = line.section(':', 0, 0).trimmed();
            QString value = line.section(':', 1).trimmed();

            if (currentBlock == "editor") {
                if (key == "foreground") theme.foreground = parseColor(value);
                else if (key == "background") theme.background = parseColor(value);
                else if (key == "caret") theme.caret = parseColor(value);
            } else if (currentBlock == "editor-selection") {
                if (key == "foreground") theme.selectionForeground = parseColor(value);
                else if (key == "background") theme.selectionBackground = parseColor(value);
            } else {
                if (key == "foreground")
                    currentFormat.setForeground(parseColor(value));
                else if (key == "background")
                    currentFormat.setBackground(parseColor(value));
                else if (key == "font-style") {
                    if (value.contains("bold"))
                        currentFormat.setFontWeight(QFont::Bold);
                    if (value.contains("italic"))
                        currentFormat.setFontItalic(true);
                }
                // font-size is parsed but applied relative to base font
            }
        } else {
            // New block name
            currentBlock = line.trimmed();
            currentFormat = QTextCharFormat();
            inElement = true;
        }
    }

    // Handle last block
    if (inElement && !currentBlock.isEmpty() &&
        currentBlock != "editor" && currentBlock != "editor-selection") {
        theme.formats[currentBlock] = currentFormat;
    }

    return theme;
}
