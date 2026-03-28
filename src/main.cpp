#include <QApplication>
#include <QIcon>
#include "MainWindow.h"
#include "Preferences.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("WinDown 3000");
    app.setApplicationVersion("3000.0.4");
    app.setOrganizationName("WinDown3000");
    app.setOrganizationDomain("windown3000.app");
    app.setWindowIcon(QIcon(":/icons/windown3000.png"));

    // Apply saved theme
    Preferences prefs;
    if (prefs.editorStyleName().contains("Night") ||
        prefs.editorStyleName().contains("Dark") ||
        prefs.editorStyleName().contains("Tomorrow+")) {
        app.setStyle("Fusion");
    }

    MainWindow window;
    window.resize(1200, 800);
    window.show();

    // Open file from command line argument
    if (argc > 1) {
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
