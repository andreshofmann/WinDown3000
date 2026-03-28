# WinDown 3000

### The fastest way to view and edit Markdown.

[![Build](https://github.com/andreshofmann/WinDown3000/workflows/Build/badge.svg)](https://github.com/andreshofmann/WinDown3000/actions)
[![GitHub release](https://img.shields.io/github/v/release/andreshofmann/WinDown3000)](https://github.com/andreshofmann/WinDown3000/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/andreshofmann/WinDown3000/total)](https://github.com/andreshofmann/WinDown3000/releases)

---

### Download

| Platform | Download | Notes |
|----------|----------|-------|
| **macOS** | [WinDown3000-macOS.dmg](https://github.com/andreshofmann/WinDown3000/releases/latest/download/WinDown3000-macOS.dmg) | Drag to Applications |
| **Windows** | [WinDown3000-SingleFile.exe](https://github.com/andreshofmann/WinDown3000/releases/latest/download/WinDown3000-SingleFile.exe) | Single file, no install needed |
| Windows (zip) | [WinDown3000-Windows-x64.zip](https://github.com/andreshofmann/WinDown3000/releases/latest/download/WinDown3000-Windows-x64.zip) | Extract and run |
| Windows (installer) | [WinDown3000-Setup.exe](https://github.com/andreshofmann/WinDown3000/releases/latest) | Start Menu + .md file association |

---

## What is WinDown 3000?

WinDown 3000 is a free, lightweight **Markdown viewer and editor** for macOS and Windows. Open any `.md` file and instantly see a beautifully rendered preview alongside the source. No bloat, no account required, no internet needed.

Built with C++ and Qt for native speed. Based on [MacDown 3000](https://github.com/schuyler/macdown3000), the popular macOS Markdown editor.

### Use it to

- **View** README files, documentation, notes, and any Markdown content with live-rendered preview
- **Edit** Markdown with syntax highlighting, auto-completion, and instant preview
- **Export** your documents to HTML or PDF
- **Present** technical docs with code highlighting, math notation, and diagrams

### Why WinDown 3000?

| | WinDown 3000 | VS Code | Notepad++ | Online editors |
|---|---|---|---|---|
| Single file, no install | Yes | No | No | N/A |
| Instant live preview | Yes | Extension needed | No | Some |
| Math/diagram support | Yes | Extension needed | No | Some |
| Works offline | Yes | Yes | Yes | No |
| Lightweight | ~30 MB | ~400 MB | ~5 MB | N/A |
| Free & open source | Yes | Yes | Yes | Varies |

## Features

### Instant Markdown Preview

Open a file and see it rendered in real time, side-by-side with the source. The split-pane view with synchronized scrolling makes it easy to read and edit at the same time.

### Full Markdown Support

- **GitHub Flavored Markdown** — tables, strikethrough, autolinks, task lists
- **Fenced code blocks** with language identifiers
- **Footnotes**, superscript, highlight, and underline extensions
- **CommonMark** compatible

### Code Syntax Highlighting

Code blocks are highlighted in both the editor and preview using [Prism](https://prismjs.com), supporting 200+ programming languages.

### Math, Diagrams, and More

- **Math notation** via MathJax — `$$E=mc^2$$`, `\[...\]`, `\(...\)`
- **Mermaid diagrams** — flowcharts, sequence diagrams, Gantt charts
- **Graphviz** — dot language diagrams
- **Jekyll front-matter** detection

### Editor Features

- **15 editor themes** — Tomorrow, Solarized, Mou, Writer, and more
- **8 preview CSS themes** — GitHub, Clearness, Solarized
- **Auto-completion** — bracket/quote pairing, smart list continuation
- **Find and Replace** — with regex, case-sensitive, and whole-word options
- **Formatting toolbar** — bold, italic, headings, lists, links, code
- **Keyboard shortcuts** — full set for efficient editing
- **Word count** — live word and character count
- **Drag-and-drop** — open files by dropping onto the window
- **Export** — HTML and PDF

### Customizable

Full preferences dialog with control over every aspect: font, theme, margins, Markdown extensions, preview rendering, code highlighting, and more.

## System Requirements

- **macOS** 11.0 (Big Sur) or later
- **Windows** 10 or later (x64)
- No other dependencies

## Building from Source

### Requirements

- CMake 3.21+
- Qt 6.5+ (with WebEngine module)
- C++17 compiler (MSVC 2019+)

### Build

```bash
git clone https://github.com/andreshofmann/WinDown3000.git
cd WinDown3000
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure
```

## Architecture

WinDown 3000 is a ground-up C++/Qt port of MacDown 3000 (Objective-C/Cocoa):

| Component | Technology |
|---|---|
| Markdown parsing | [Hoedown](https://github.com/hoedown/hoedown) (C library) |
| Editor | QPlainTextEdit + QSyntaxHighlighter |
| Preview | WebView2 (Windows) / QWebEngineView (dev) |
| Code highlighting | [Prism.js](https://prismjs.com) |
| Math rendering | [MathJax](https://www.mathjax.org) |
| Diagrams | [Mermaid](https://mermaid.js.org) + Graphviz |
| Settings | QSettings |

## Testing

128 tests across 6 test suites run on every commit:

```
test_markdown_renderer  — 35 tests (rendering, extensions, templates)
test_theme_parser       —  9 tests (theme file parsing)
test_preferences        — 35 tests (defaults, persistence, signals)
test_document           — 16 tests (file I/O, state management)
test_editor             — 20 tests (editing, auto-complete, formatting)
test_find_replace       — 13 tests (search, replace, regex)
```

## License

MIT License. See the `LICENSE` directory for details.

## Credits

- [MacDown 3000](https://github.com/schuyler/macdown3000) by Schuyler Erle
- [MacDown](https://macdown.uranusjr.com) by Tzu-ping Chung
- [Mou](http://25.io/mou/) by Chen Luo

## Contributing

Contributions welcome! [File an issue](https://github.com/andreshofmann/WinDown3000/issues/new) for bugs, features, or questions.
