#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>

#include "database.hpp"
#include "loginwindow.hpp"
#include "mainwindow.hpp"

int main(int argc, char* argv[]) {
    qputenv("QT_LOGGING_RULES", "qt.qpa.wayland.textinput=false");
    QApplication app(argc, argv);
    app.setApplicationName("Tella POS");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Tella POS");

    // Load stylesheet from resources
    QFile styleFile(":/style.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    }

    // Determine database path in user's app data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + "/tella.db";

    // Open database
    if (!Database::instance().open(dbPath)) {
        QMessageBox::critical(
            nullptr, "Database Error",
            QString("Failed to open database:\n%1\n\nPath: %2").arg(Database::instance().lastError(), dbPath));
        return 1;
    }

    LoginWindow login;
    if (login.exec() != QDialog::Accepted) {
        return 1;
    }

    User loggedInUser = login.loggedInUser();
    auto* mainWin = new MainWindow(loggedInUser);
    mainWin->showMaximized();
    app.exec();
    delete mainWin;
    Database::instance().close();
    return 0;
}
