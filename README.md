# WinDown 3000

[![Windows Build](https://github.com/andreshofmann/WinDown3000/workflows/Windows%20Build/badge.svg)](https://github.com/andreshofmann/WinDown3000/actions)

WinDown 3000 is a free, lightweight Markdown editor for Windows with live preview. It is a port of [MacDown 3000](https://github.com/schuyler/macdown3000), which continues the legacy started by Chen Luo's [Mou](http://25.io/mou/) and carried forward by Tzu-ping Chung's [MacDown](https://macdown.uranusjr.com).

Download from the [releases](https://github.com/andreshofmann/WinDown3000/releases) page.

## About WinDown 3000

WinDown 3000 brings the MacDown experience to Windows. Built with Qt and C++, it is fast, portable, and ships as a single executable with no installer required.

### System Requirements

- Windows 10 or later (x64)
- Microsoft Edge WebView2 Runtime (pre-installed on Windows 10/11)

## Features

### Live Preview & Markdown Rendering

WinDown 3000 uses [Hoedown](https://github.com/hoedown/hoedown) to convert Markdown to HTML with live preview as you type. It supports:

- **GitHub Flavored Markdown** including tables, strikethrough, and autolinks
- **Fenced code blocks** with language identifiers
- **Task lists** with interactive checkboxes
- **Footnotes**, superscript, highlight, and underline extensions
- **Customizable rendering options** in Preferences

### Syntax Highlighting

- **Editor**: Markdown syntax highlighting with 15 editor themes ported from MacDown
- **Preview**: Code block highlighting via [Prism](https://prismjs.com) supporting 200+ languages

### Additional Rendering

- **Math notation** via MathJax (`$$...$$`, `\[...\]`, `\(...\)`, optional `$...$`)
- **Diagrams** via Mermaid (flowcharts, sequence, Gantt) and Graphviz
- **Jekyll front-matter** support
- **Export to HTML or PDF**

### Editor Features

- **Auto-completion**: Bracket/quote pairing, smart list continuation, auto-numbered lists
- **Scroll sync**: Bidirectional editor-preview scroll synchronization
- **Smart paste**: Paste a URL over selected text to create a Markdown link
- **Formatting toolbar**: Bold, italic, headings, lists, links, images, code
- **Keyboard shortcuts**: Full set matching MacDown 3000
- **Drag-and-drop**: Open files by dropping them onto the window
- **Word count**: Live word and character count in the status bar

### Themes

**Editor themes** (ported from MacDown 3000):
Mou Fresh Air, Mou Night, Mou Paper, Solarized Light/Dark, Tomorrow, Tomorrow Blue, Writer (+ enhanced variants)

**Preview CSS themes**:
GitHub, GitHub2, GitHub 2020, GitHub Tomorrow (dark), Clearness, Clearness Dark, Solarized Light/Dark

## Building from Source

### Requirements

- CMake 3.21+
- Qt 6.5+ (with WebEngine module for development builds)
- C++17 compiler (MSVC 2019+ recommended on Windows)
- Git

### Build (Windows)

```bash
git clone https://github.com/andreshofmann/WinDown3000.git
cd WinDown3000
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Build (Development on macOS/Linux)

For development, the app falls back to QWebEngineView instead of WebView2:

```bash
cmake -B build -DWINDOWN_USE_WEBVIEW2=OFF
cmake --build build
```

### Static Single-Exe Build

For a fully portable single executable:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DWINDOWN_STATIC_BUILD=ON -DCMAKE_PREFIX_PATH=/path/to/static-qt
cmake --build build --config Release
```

## Architecture

WinDown 3000 is a ground-up port of MacDown 3000 from Objective-C/Cocoa to C++/Qt:

| MacDown 3000 | WinDown 3000 | Purpose |
|---|---|---|
| NSTextView | QPlainTextEdit | Markdown editor |
| WebView (WebKit) | WebView2 / QWebEngineView | Live preview |
| PEG Markdown Highlight | QSyntaxHighlighter | Editor highlighting |
| Hoedown | Hoedown | Markdown-to-HTML |
| Prism.js | Prism.js | Code highlighting |
| MathJax | MathJax | Math rendering |
| NSUserDefaults | QSettings | Preferences |
| Cocoa/AppKit | Qt Widgets | UI framework |

## License

WinDown 3000 is released under the MIT License. See the `LICENSE` directory for details.

Editor themes and CSS files from [Mou](http://mouapp.com) are courtesy of Chen Luo.

## Credits

- [MacDown 3000](https://github.com/schuyler/macdown3000) by Schuyler Erle
- [MacDown](https://macdown.uranusjr.com) by Tzu-ping Chung
- [Mou](http://25.io/mou/) by Chen Luo
- [Hoedown](https://github.com/hoedown/hoedown) for Markdown parsing
- [Prism](https://prismjs.com) for syntax highlighting
- [MathJax](https://www.mathjax.org) for math rendering
- [Mermaid](https://mermaid.js.org) for diagrams

## Contributing

Contributions are welcome! Please [file an issue](https://github.com/andreshofmann/WinDown3000/issues/new) for bug reports, feature requests, or questions.
