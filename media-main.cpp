#include "mediacontroller.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MPRIS Media Controller"));
    QApplication::setQuitOnLastWindowClosed(false);

    const bool toggle = application.arguments().contains(
        QStringLiteral("--toggle"));

    if (toggle &&
        QDBusConnection::sessionBus().interface()->isServiceRegistered(
            QStringLiteral("org.MprisMediaController.standalone"))) {
        QDBusInterface controller(
            QStringLiteral("org.MprisMediaController.standalone"),
            QStringLiteral("/MediaController"),
            QStringLiteral("org.MprisMediaController.standalone"),
            QDBusConnection::sessionBus());
        controller.call(QStringLiteral("toggleVisibility"));
        return 0;
    }

    QDBusConnection::sessionBus().registerService(
        QStringLiteral("org.MprisMediaController.standalone"));

    MediaController controller;
    controller.show();

    return application.exec();
}
