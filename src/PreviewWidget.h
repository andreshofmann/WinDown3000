#pragma once

#include <QWidget>
#include <QString>

class Preferences;

#ifdef WINDOWN_USE_WEBVIEW2
// Forward-declare the WebView2 COM interfaces (Windows only)
struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;
#else
#include <QWebEngineView>
#endif

/// HTML preview widget.
/// Uses Microsoft WebView2 on Windows (lightweight, uses system Edge),
/// or QWebEngineView as fallback on other platforms.
class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWidget(Preferences *prefs, QWidget *parent = nullptr);
    ~PreviewWidget() override;

    /// Set the full HTML content to display.
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl());

    /// Execute JavaScript in the preview context.
    void runJavaScript(const QString &script);

    /// Get the current scroll position (async via signal).
    void requestScrollPosition();

    /// Set the scroll position (0.0 to 1.0).
    void setScrollPosition(qreal fraction);

    /// Print the current content to a PDF file.
    void printToPdf(const QString &filePath);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void scrollPositionChanged(qreal fraction);
    void linkClicked(const QUrl &url);
    void checkboxToggled(int index);
    void textDoubleClicked(const QString &text);
    void pdfExportFinished(bool success);

private:
    void initWebView();

    Preferences *m_prefs;

#ifdef WINDOWN_USE_WEBVIEW2
    ICoreWebView2Controller *m_controller = nullptr;
    ICoreWebView2 *m_webview = nullptr;
    HWND m_hwnd = nullptr;
#else
    QWebEngineView *m_webView = nullptr;
#endif
};
