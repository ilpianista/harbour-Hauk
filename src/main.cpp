#include <QtQuick>

#include <sailfishapp.h>

#include "haukclient.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    QCoreApplication::setApplicationName(QStringLiteral("harbour-hauk"));
    QCoreApplication::setOrganizationName(QStringLiteral("dev.scarpino"));

    qmlRegisterType<HaukClient>("harbour.hauk", 1, 0, "HaukClient");

    view->setSource(SailfishApp::pathTo("qml/Hauk.qml"));
    view->show();

    return app->exec();
}
