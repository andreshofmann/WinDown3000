#include "PreviewWidget.h"
#include "Preferences.h"

#include <QVBoxLayout>
#include <QUrl>

#ifdef WINDOWN_USE_WEBVIEW2
// =========================================================================
// WebView2 implementation (Windows)
// =========================================================================
#include <wil/com.h>
#include <WebView2.h>
#include <windows.h>

PreviewWidget::PreviewWidget(Preferences *prefs, QWidget *parent)
    : QWidget(parent)
    , m_prefs(prefs)
{
    initWebView();
}

PreviewWidget::~PreviewWidget()
{
    if (m_controller) m_controller->Release();
}

void PreviewWidget::initWebView()
{
    m_hwnd = reinterpret_cast<HWND>(winId());

    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT, ICoreWebView2Environment *env) -> HRESULT {
                env->CreateCoreWebView2Controller(m_hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT, ICoreWebView2Controller *controller) -> HRESULT {
                            m_controller = controller;
                            m_controller->get_CoreWebView2(&m_webview);

                            // Resize to fill widget
                            RECT bounds;
                            GetClientRect(m_hwnd, &bounds);
                            m_controller->put_Bounds(bounds);

                            // Handle navigation for checkbox toggles
                            m_webview->add_NavigationStarting(
                                Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [this](ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT {
                                        LPWSTR uri;
                                        args->get_Uri(&uri);
                                        QString url = QString::fromWCharArray(uri);
                                        CoTaskMemFree(uri);

                                        if (url.startsWith("x-windown-checkbox://toggle/")) {
                                            args->put_Cancel(TRUE);
                                            int idx = url.mid(28).toInt();
                                            emit checkboxToggled(idx);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

void PreviewWidget::setHtml(const QString &html, const QUrl &)
{
    if (m_webview) {
        m_webview->NavigateToString(html.toStdWString().c_str());
    }
}

void PreviewWidget::runJavaScript(const QString &script)
{
    if (m_webview) {
        m_webview->ExecuteScript(script.toStdWString().c_str(), nullptr);
    }
}

void PreviewWidget::requestScrollPosition()
{
    runJavaScript(QStringLiteral(
        "window.chrome.webview.postMessage({"
        "  type: 'scroll',"
        "  position: document.documentElement.scrollTop / "
        "    (document.documentElement.scrollHeight - document.documentElement.clientHeight)"
        "});"));
}

void PreviewWidget::setScrollPosition(qreal fraction)
{
    runJavaScript(QStringLiteral(
        "document.documentElement.scrollTop = %1 * "
        "(document.documentElement.scrollHeight - document.documentElement.clientHeight);")
        .arg(fraction));
}

#else
// =========================================================================
// QWebEngineView implementation (fallback for non-Windows / development)
// =========================================================================
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>

PreviewWidget::PreviewWidget(Preferences *prefs, QWidget *parent)
    : QWidget(parent)
    , m_prefs(prefs)
    , m_webView(new QWebEngineView(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    // Handle navigation for checkbox toggles
    connect(m_webView->page(), &QWebEnginePage::urlChanged, this, [this](const QUrl &url) {
        if (url.scheme() == "x-windown-checkbox" && url.host() == "toggle") {
            int idx = url.path().mid(1).toInt(); // strip leading /
            emit checkboxToggled(idx);
        }
    });

    initWebView();
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::initWebView() {}

void PreviewWidget::setHtml(const QString &html, const QUrl &baseUrl)
{
    m_webView->setHtml(html, baseUrl);
}

void PreviewWidget::runJavaScript(const QString &script)
{
    m_webView->page()->runJavaScript(script);
}

void PreviewWidget::requestScrollPosition()
{
    m_webView->page()->runJavaScript(
        QStringLiteral(
            "document.documentElement.scrollTop / "
            "(document.documentElement.scrollHeight - document.documentElement.clientHeight)"),
        [this](const QVariant &result) {
            emit scrollPositionChanged(result.toReal());
        });
}

void PreviewWidget::setScrollPosition(qreal fraction)
{
    runJavaScript(QStringLiteral(
        "document.documentElement.scrollTop = %1 * "
        "(document.documentElement.scrollHeight - document.documentElement.clientHeight);")
        .arg(fraction));
}

#endif // WINDOWN_USE_WEBVIEW2
