#include "PreviewWidget.h"
#include "Preferences.h"

#include <QResizeEvent>
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

void PreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_controller) {
        RECT bounds;
        GetClientRect(m_hwnd, &bounds);
        m_controller->put_Bounds(bounds);
    }
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

void PreviewWidget::printToPdf(const QString &filePath)
{
    if (m_webview) {
        // WebView2 PrintToPdf API
        wil::com_ptr<ICoreWebView2_7> webview7;
        if (SUCCEEDED(m_webview->QueryInterface(IID_PPV_ARGS(&webview7)))) {
            webview7->PrintToPdf(filePath.toStdWString().c_str(), nullptr,
                Microsoft::WRL::Callback<ICoreWebView2PrintToPdfCompletedHandler>(
                    [this](HRESULT hr, BOOL success) -> HRESULT {
                        emit pdfExportFinished(SUCCEEDED(hr) && success);
                        return S_OK;
                    }).Get());
        }
    }
}

#else
// =========================================================================
// QWebEngineView implementation (fallback for non-Windows / development)
// =========================================================================
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineSettings>
#include <QDesktopServices>
#include <QPageLayout>
#include <QPageSize>

/// Custom page that blocks all external navigation.
/// Only allows setHtml content loads. Link clicks open in system browser.
class PreviewPage : public QWebEnginePage
{
    Q_OBJECT
public:
    using QWebEnginePage::QWebEnginePage;
signals:
    void checkboxToggled(int index);
    void linkClicked(const QUrl &url);
protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        Q_UNUSED(isMainFrame);

        // Handle checkbox toggle scheme
        if (url.scheme() == "x-windown-checkbox") {
            if (url.host() == "toggle") {
                int idx = url.path().mid(1).toInt();
                emit checkboxToggled(idx);
            }
            return false;
        }

        // Allow setHtml content loads (blank page or data URL)
        if (type == NavigationTypeTyped)
            return true;

        // Allow initial page load of about:blank or data: content
        if (url.scheme() == "data" || url.toString() == "about:blank")
            return true;

        // Everything else (link clicks, redirects, etc.) — block navigation
        // Open http/https links in system browser
        if (url.scheme() == "http" || url.scheme() == "https") {
            QDesktopServices::openUrl(url);
        }
        emit linkClicked(url);
        return false;
    }
};

PreviewWidget::PreviewWidget(Preferences *prefs, QWidget *parent)
    : QWidget(parent)
    , m_prefs(prefs)
    , m_webView(new QWebEngineView(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    // Use custom page that blocks link navigation
    auto *page = new PreviewPage(m_webView);
    m_webView->setPage(page);

    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    // Forward signals from custom page
    connect(page, &PreviewPage::checkboxToggled, this, &PreviewWidget::checkboxToggled);
    connect(page, &PreviewPage::linkClicked, this, &PreviewWidget::linkClicked);

    // Block popups / target="_blank" links
    connect(page, &QWebEnginePage::newWindowRequested, this, [](QWebEngineNewWindowRequest &req) {
        QUrl url = req.requestedUrl();
        if (url.scheme() == "http" || url.scheme() == "https")
            QDesktopServices::openUrl(url);
        // Don't create a new window — just ignore the request
    });

    // Block any navigation that slips through (e.g. JS window.location)
    connect(m_webView, &QWebEngineView::urlChanged, this, [this](const QUrl &url) {
        // If the URL changed to something other than blank/data, block it
        if (url.scheme() == "http" || url.scheme() == "https") {
            QDesktopServices::openUrl(url);
            // Reload the current content to undo the navigation
            m_webView->stop();
        }
    });

    initWebView();
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::initWebView() {}

// Include the MOC for the PreviewPage class defined in this .cpp
#include "PreviewWidget.moc"

void PreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

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

void PreviewWidget::printToPdf(const QString &filePath)
{
    m_webView->page()->printToPdf(filePath, QPageLayout(
        QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(15, 15, 15, 15)));
    connect(m_webView->page(), &QWebEnginePage::pdfPrintingFinished,
            this, [this](const QString &, bool success) {
                emit pdfExportFinished(success);
            });
}

#endif // WINDOWN_USE_WEBVIEW2
