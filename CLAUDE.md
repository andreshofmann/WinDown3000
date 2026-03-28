# WinDown 3000

A fast, lightweight Markdown editor for Windows, ported from MacDown 3000.

## Development Paradigm

**Test first, release next.** Every aspect of this application must be tested before any release.

### Testing Requirements
- **Unit tests** must cover all core logic: MarkdownRenderer, ThemeParser, Preferences, Document model, Editor behavior (bracket pairing, list continuation, word count), checkbox toggling, front-matter stripping
- **Integration tests** must verify the full render pipeline: markdown input → Hoedown rendering → HTML output with correct styles/scripts
- **E2E tests** must exercise the UI: MainWindow lifecycle, file open/save/export, preferences dialog, find/replace, toolbar actions, scroll sync, drag-drop
- **All tests run on every commit** via CI — builds that fail tests must not be released
- **New features require tests** before they can be merged

### Test Framework
- Google Test (gtest) for unit and integration tests
- Qt Test (QTest) for widget/UI tests
- Tests live in `tests/` directory
- CI runs `ctest` after build — any failure blocks the release job

## Architecture

- **Language**: C++17 with Qt 6
- **Build system**: CMake 3.21+
- **Markdown parser**: Hoedown 3.0.7 (C library, fetched via CMake FetchContent)
- **Preview**: WebView2 on Windows, QWebEngineView fallback
- **Preferences**: QSettings (Windows Registry)
- **Syntax highlighting**: QSyntaxHighlighter with MacDown .style theme format

### Key source files
- `src/MainWindow.cpp` — Main window, menus, toolbar, wiring
- `src/Document.cpp` — Document model, file I/O, render coordination
- `src/Editor.cpp` — Markdown editor with auto-completion
- `src/MarkdownRenderer.cpp` — Hoedown wrapper, HTML template generation
- `src/MarkdownHighlighter.cpp` — Editor syntax highlighting
- `src/PreviewWidget.cpp` — WebView2 / QWebEngine preview
- `src/Preferences.cpp` — Settings (40+ preferences)
- `src/PreferencesDialog.cpp` — Full tabbed preferences UI
- `src/FindReplaceDialog.cpp` — Find and replace with regex support
- `src/ThemeParser.cpp` — MacDown .style theme file parser

### Resources
- `resources/styles/` — 8 preview CSS themes (from MacDown 3000)
- `resources/themes/` — 15 editor themes (from MacDown 3000)
- `resources/extensions/` — Mermaid, Graphviz, tasklist JS

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DWINDOWN_USE_WEBVIEW2=OFF
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## CI/CD

- GitHub Actions: `.github/workflows/windows-build.yml`
- Builds on every push to main and on tags
- Tags matching `v*` trigger GitHub Release creation
- Portable zip + Inno Setup installer published to Releases
