#include "mediacontroller.h"

#include <QApplication>
#include <QComboBox>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariantMap>

namespace {

constexpr auto kRootPath = "/org/mpris/MediaPlayer2";
constexpr auto kPlayerInterface = "org.mpris.MediaPlayer2.Player";
constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr auto kPropertiesPath = "/org/mpris/MediaPlayer2";

QVariantMap metadata(const QDBusInterface &player)
{
    return player.property("Metadata").toMap();
}

QStringList mprisServices()
{
    auto *interface = QDBusConnection::sessionBus().interface();

    if (!interface)
        return {};

    const QDBusReply<QStringList> reply = interface->registeredServiceNames();

    if (!reply.isValid())
        return {};

    QStringList services;

    for (const QString &service : reply.value()) {
        if (service.startsWith(QStringLiteral("org.mpris.MediaPlayer2.")))
            services.append(service);
    }

    return services;
}

QString firstString(const QVariant &value)
{
    const QStringList values = value.toStringList();
    return values.isEmpty() ? value.toString() : values.first();
}

}

MediaController::MediaController(QWidget *parent)
    : QMainWindow(parent)
{
    QDBusConnection::sessionBus().registerObject(
        QStringLiteral("/MediaController"),
        QStringLiteral("org.tommaso.MprisMediaController"),
        this,
        QDBusConnection::ExportAllSlots);

    setWindowTitle(QStringLiteral("MPRIS Media Controller"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_QuitOnClose, false);
    resize(620, 280);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(12);

    m_playerSelector = new QComboBox(central);
    m_titleLabel = new QLabel(central);
    m_artistLabel = new QLabel(central);
    m_timeLabel = new QLabel(central);
    m_positionSlider = new QSlider(Qt::Horizontal, central);
    m_previousButton = new QPushButton(QStringLiteral("Previous"), central);
    m_playPauseButton = new QPushButton(QStringLiteral("Play"), central);
    m_nextButton = new QPushButton(QStringLiteral("Next"), central);

    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_artistLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));
    m_artistLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));
    m_positionSlider->setRange(0, 1000);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_previousButton);
    buttons->addWidget(m_playPauseButton);
    buttons->addWidget(m_nextButton);
    buttons->addStretch();

    mainLayout->addWidget(m_playerSelector);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_artistLabel);
    mainLayout->addWidget(m_positionSlider);
    mainLayout->addWidget(m_timeLabel);
    mainLayout->addLayout(buttons);
    setCentralWidget(central);

    connect(m_playerSelector, &QComboBox::currentIndexChanged,
            this, &MediaController::updatePlayerView);
    connect(m_previousButton, &QPushButton::clicked,
            this, [this]() { callPlayer(QStringLiteral("Previous")); });
    connect(m_playPauseButton, &QPushButton::clicked,
            this, [this]() { callPlayer(QStringLiteral("PlayPause")); });
    connect(m_nextButton, &QPushButton::clicked,
            this, [this]() { callPlayer(QStringLiteral("Next")); });
    connect(m_positionSlider, &QSlider::sliderReleased, this, [this]() {
        seekTo(m_positionSlider->value());
    });

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(500);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &MediaController::refreshCurrentPlayer);
    m_refreshTimer->start();

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme(QStringLiteral("multimedia-player")));
    auto *trayMenu = new QMenu(this);
    trayMenu->addAction(QStringLiteral("Show controller"), this,
                        &MediaController::showController);
    trayMenu->addAction(QStringLiteral("Quit"), qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger)
                    showController();
            });
    m_trayIcon->show();

    refreshPlayers();
    updatePlayerView();
}

bool MediaController::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate && isVisible())
        hide();

    return QMainWindow::event(event);
}

QString MediaController::currentService() const
{
    return m_playerSelector->currentData().toString();
}

void MediaController::refreshPlayers()
{
    const QString selected = currentService();
    const QStringList services = mprisServices();

    m_playerSelector->blockSignals(true);
    m_playerSelector->clear();

    for (const QString &service : services) {
        const QString name =
            service.mid(QStringLiteral("org.mpris.MediaPlayer2.").length());
        m_playerSelector->addItem(name, service);
    }

    const int selectedIndex = m_playerSelector->findData(selected);
    if (selectedIndex >= 0)
        m_playerSelector->setCurrentIndex(selectedIndex);
    else if (m_playerSelector->count() > 0)
        m_playerSelector->setCurrentIndex(0);

    m_playerSelector->blockSignals(false);
}

void MediaController::refreshCurrentPlayer()
{
    refreshPlayers();
    updatePlayerView();
}

void MediaController::updatePlayerView()
{
    const QString service = currentService();

    if (service.isEmpty()) {
        m_titleLabel->setText(QStringLiteral("No MPRIS player found"));
        m_artistLabel->clear();
        m_timeLabel->clear();
        m_playPauseButton->setEnabled(false);
        m_previousButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        return;
    }

    QDBusInterface player(service, kRootPath, kPlayerInterface,
                          QDBusConnection::sessionBus());
    const QVariantMap data = metadata(player);
    const QString status = player.property("PlaybackStatus").toString();
    const qint64 position = player.property("Position").toLongLong();
    const qint64 duration = data.value("mpris:length").toLongLong();

    m_titleLabel->setText(firstString(data.value("xesam:title")));
    m_artistLabel->setText(firstString(data.value("xesam:artist")));
    m_playPauseButton->setText(status == QStringLiteral("Playing")
                                    ? QStringLiteral("Pause")
                                    : QStringLiteral("Play"));
    m_timeLabel->setText(formatTime(position) + QStringLiteral(" / ")
                         + formatTime(duration));
    m_updatingSlider = true;
    m_positionSlider->setValue(duration > 0
                                    ? static_cast<int>(position * 1000 / duration)
                                    : 0);
    m_updatingSlider = false;
    m_playPauseButton->setEnabled(true);
    m_previousButton->setEnabled(true);
    m_nextButton->setEnabled(true);
}

void MediaController::callPlayer(const QString &method)
{
    QDBusInterface player(currentService(), kRootPath, kPlayerInterface,
                          QDBusConnection::sessionBus());
    if (player.isValid())
        player.call(method);
}

void MediaController::seekTo(int position)
{
    if (m_updatingSlider)
        return;

    QDBusInterface player(currentService(), kRootPath, kPlayerInterface,
                          QDBusConnection::sessionBus());
    const QVariantMap data = metadata(player);
    const qint64 duration = data.value("mpris:length").toLongLong();
    const QVariant trackId = data.value("mpris:trackid");

    if (!player.isValid() || duration <= 0)
        return;

    const QDBusObjectPath path =
        trackId.canConvert<QDBusObjectPath>()
            ? trackId.value<QDBusObjectPath>()
            : QDBusObjectPath(trackId.toString());
    player.call(QStringLiteral("SetPosition"), path,
                duration * position / 1000);
}

QString MediaController::formatTime(qint64 microseconds) const
{
    const qint64 seconds = qMax<qint64>(0, microseconds / 1000000);
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

void MediaController::showController()
{
    show();
    raise();
    activateWindow();
}

void MediaController::toggleVisibility()
{
    if (isVisible())
        hide();
    else
        showController();
}
