// Entry point for the client_sim Qt6 application.

#include <QApplication>
#include <QIcon>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("client_sim"));
    QCoreApplication::setApplicationName(QStringLiteral("client_sim"));

    client_sim::ui::MainWindow w;
    w.show();
    return app.exec();
}
