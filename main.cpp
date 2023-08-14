#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QScreen>

#include "window.h"

int main(int argc, char **argv) {
    // Create the application
    QApplication app(argc, argv);
    // Import its styling
    QFile styleFile(":/style.qss");
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());
    app.setStyleSheet(style);
    // Import icon
    QIcon icon(":/icon.png");
    app.setWindowIcon(icon);

    // Create the main application window
    Window window;
    // Set the window's initial size to match the screen size
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    const QRect availableGeometry = primaryScreen->availableGeometry();
    const QSize screenSize = availableGeometry.size();
    window.resize(screenSize);
    // Show the window
    window.show();

    return app.exec();
}
