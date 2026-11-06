#include "mediacontroller.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MPRIS Media Controller"));
    QApplication::setQuitOnLastWindowClosed(false);

    MediaController controller;
    controller.show();

    return application.exec();
}
