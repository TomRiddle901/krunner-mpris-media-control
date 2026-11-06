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
            QStringLiteral("org.tommaso.MprisMediaController"))) {
        QDBusInterface controller(
            QStringLiteral("org.tommaso.MprisMediaController"),
            QStringLiteral("/MediaController"),
            QStringLiteral("org.tommaso.MprisMediaController"),
            QDBusConnection::sessionBus());
        controller.call(QStringLiteral("toggleVisibility"));
        return 0;
    }

    QDBusConnection::sessionBus().registerService(
        QStringLiteral("org.tommaso.MprisMediaController"));

    MediaController controller;
    controller.show();

    return application.exec();
}
