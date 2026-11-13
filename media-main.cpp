
#include "mediacontroller.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

namespace {

const QString kServiceName =
    QStringLiteral("org.MprisMediaController.standalone");

const QString kObjectPath =
    QStringLiteral("/MediaController");

}

int main(int argc, char **argv)
{
    QApplication application(argc, argv);

    QApplication::setApplicationName(
        QStringLiteral("MPRIS Media Controller"));

    QApplication::setQuitOnLastWindowClosed(false);

    const bool toggle =
        application.arguments().contains(QStringLiteral("--toggle"));

    QDBusConnection bus = QDBusConnection::sessionBus();

    if (!bus.isConnected()) {
        qCritical() << "Impossibile connettersi al session bus D-Bus.";
        return 1;
    }

    auto *busInterface = bus.interface();

    if (!busInterface) {
        qCritical() << "Interfaccia D-Bus non disponibile.";
        return 1;
    }

    const QDBusReply<bool> registered =
        busInterface->isServiceRegistered(kServiceName);

    if (!registered.isValid()) {
        qCritical() << "Errore durante il controllo del servizio D-Bus:"
                    << registered.error().message();
        return 1;
    }

    if (registered.value()) {
        if (!toggle)
            return 0;

        QDBusInterface controller(
            kServiceName,
            kObjectPath,
            kServiceName,
            bus);

        if (!controller.isValid()) {
            qCritical() << "Controller D-Bus non disponibile.";
            return 1;
        }

        const QDBusReply<void> reply =
            controller.call(QStringLiteral("toggleVisibility"));

        if (!reply.isValid()) {
            qCritical() << "Errore durante il comando toggle:"
                        << reply.error().message();
            return 1;
        }

        return 0;
    }

    if (toggle) {
        // Nessuna istanza attiva: avviamo normalmente il programma.
    }

    const auto registration =
    busInterface->registerService(kServiceName);

    if (!registration.isValid()) {
        qCritical() << "Impossibile registrare il servizio D-Bus:"
                    << registration.error().message();
        return 1;
    }

    if (registration.value() !=
        QDBusConnectionInterface::ServiceRegistered) {
        qCritical() << "Il servizio D-Bus non è stato registrato.";
        return 1;
    }

    MediaController controller;
    controller.show();

    return application.exec();
}