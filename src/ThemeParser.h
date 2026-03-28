#pragma once

#include <QColor>
#include <QFont>
#include <QHash>
#include <QString>
#include <QTextCharFormat>

/// Parses MacDown .style theme files into QTextCharFormats.
/// Theme file format:
///   ELEMENT_NAME
///   foreground: RRGGBB
///   background: RRGGBB
///   font-style: bold|italic
///   font-size: NNpx
struct EditorTheme
{
    QColor foreground{0xCC, 0xCC, 0xCC};
    QColor background{0x2D, 0x2D, 0x2D};
    QColor caret{0xCC, 0x99, 0xCC};
    QColor selectionForeground{0xFF, 0xFF, 0xFF};
    QColor selectionBackground{0x51, 0x51, 0x51};

    /// Format for each markdown element: H1, H2, EMPH, STRONG, CODE, etc.
    QHash<QString, QTextCharFormat> formats;

    /// Load a theme from a .style resource path.
    static EditorTheme load(const QString &path);
};
