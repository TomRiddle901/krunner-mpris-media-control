#include "mediacontroller.h"

#include <QApplication>
#include <QComboBox>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QEvent>
#include <QFile>
#include <QHBoxLayout>
#include <QImage>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QStringList>
#include <QUrl>
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
    if (value.userType() == qMetaTypeId<QDBusArgument>()) {
        const QDBusArgument argument = value.value<QDBusArgument>();
        QStringList values;
        argument >> values;

        if (!values.isEmpty())
            return values.first();
    } else if (value.canConvert<QStringList>()) {
        const QStringList values = value.toStringList();

        if (!values.isEmpty())
            return values.first();
    } else if (value.canConvert<QVariantList>()) {
        const QVariantList values = value.toList();

        if (!values.isEmpty())
            return values.first().toString();
    }

    return value.toString();
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
    resize(620, 380);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(12);

    m_playerSelector = new QComboBox(central);
    m_titleLabel = new QLabel(central);
    m_artistLabel = new QLabel(central);
    m_technicalLabel = new QLabel(central);
    m_artworkLabel = new QLabel(central);
    m_timeLabel = new QLabel(central);
    m_positionSlider = new QSlider(Qt::Horizontal, central);
    m_previousButton = new QPushButton(QStringLiteral("Previous"), central);
    m_playPauseButton = new QPushButton(QStringLiteral("Play"), central);
    m_nextButton = new QPushButton(QStringLiteral("Next"), central);

    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_artistLabel->setAlignment(Qt::AlignCenter);
    m_technicalLabel->setAlignment(Qt::AlignCenter);
    m_artworkLabel->setFixedSize(180, 180);
    m_artworkLabel->setAlignment(Qt::AlignCenter);
    m_artworkLabel->setStyleSheet(
        QStringLiteral("background: palette(alternate-base); border-radius: 8px;"));
    m_playerSelector->setIconSize(QSize(20, 20));
    m_playerSelector->setMaximumWidth(240);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));
    m_artistLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));
    m_technicalLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));
    m_positionSlider->setRange(0, 1000);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_previousButton);
    buttons->addWidget(m_playPauseButton);
    buttons->addWidget(m_nextButton);
    buttons->addStretch();

    auto *detailsLayout = new QVBoxLayout;
    detailsLayout->addWidget(m_titleLabel);
    detailsLayout->addWidget(m_artistLabel);
    detailsLayout->addWidget(m_technicalLabel);

    auto *contentLayout = new QHBoxLayout;
    contentLayout->addWidget(m_artworkLabel);
    contentLayout->addLayout(detailsLayout, 1);

    mainLayout->addWidget(m_playerSelector, 0, Qt::AlignLeft);
    mainLayout->addLayout(contentLayout);
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
        m_playerSelector->addItem(playerIcon(service), name, service);
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
        m_technicalLabel->clear();
        m_artworkLabel->setPixmap({});
        m_artworkLabel->setText(QStringLiteral("No artwork"));
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
    const QString artist = firstString(data.value("xesam:artist"));
    const QString album =
        firstString(data.value("xesam:album")).isEmpty()
            ? firstString(data.value("xesam:albumArtist"))
            : firstString(data.value("xesam:album"));

    m_titleLabel->setText(firstString(data.value("xesam:title")));
    if (!artist.isEmpty() && !album.isEmpty())
        m_artistLabel->setText(
            artist + QStringLiteral(" · ") + album);
    else
        m_artistLabel->setText(artist.isEmpty() ? album : artist);
    m_technicalLabel->setText(technicalMetadata(data));
    updateArtwork(data);
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

QString MediaController::technicalMetadata(const QVariantMap &data) const
{
    QStringList values;

    const QString audioCodec =
        firstString(data.value(QStringLiteral("mpris:audioCodec")));
    const QString videoCodec =
        firstString(data.value(QStringLiteral("mpris:videoCodec")));
    const QString bitrate =
        firstString(data.value(QStringLiteral("mpris:audioBitrate")));
    const QString videoBitrate =
        firstString(data.value(QStringLiteral("mpris:videoBitrate")));
    const QString videoWidth =
        firstString(data.value(QStringLiteral("mpris:videoWidth")));
    const QString videoHeight =
        firstString(data.value(QStringLiteral("mpris:videoHeight")));
    const QString channels =
        firstString(data.value(QStringLiteral("mpris:audioChannels")));

    if (!audioCodec.isEmpty())
        values.append(audioCodec);
    if (!videoCodec.isEmpty())
        values.append(videoCodec);
    if (!bitrate.isEmpty())
        values.append(bitrate + QStringLiteral(" kbps"));
    if (!videoBitrate.isEmpty())
        values.append(videoBitrate + QStringLiteral(" kbps"));
    if (!videoWidth.isEmpty() && !videoHeight.isEmpty())
        values.append(videoWidth + QStringLiteral("x") + videoHeight);
    if (!channels.isEmpty())
        values.append(channels + QStringLiteral(" ch"));

    return values.join(QStringLiteral(" · "));
}

void MediaController::updateArtwork(const QVariantMap &data)
{
    QPixmap artwork;
    const QString artUrlValue =
        data.value(QStringLiteral("mpris:artUrl")).toString();
    const QUrl artUrl(artUrlValue);

    if (artUrl.isLocalFile())
        artwork.load(artUrl.toLocalFile());
    else if (artUrlValue.startsWith(QStringLiteral("data:"))) {
        const qsizetype separator = artUrlValue.indexOf(QLatin1Char(','));

        if (separator > 0) {
            const QString header = artUrlValue.left(separator);
            const QString payload = artUrlValue.mid(separator + 1);
            const QByteArray imageData =
                header.contains(QStringLiteral(";base64"),
                               Qt::CaseInsensitive)
                    ? QByteArray::fromBase64(payload.toLatin1())
                    : QUrl::fromPercentEncoding(
                          payload.toLatin1()).toUtf8();

            artwork.loadFromData(imageData);
        }
    }

    if (artwork.isNull()) {
        m_artworkLabel->setPixmap(
            QIcon::fromTheme(QStringLiteral("multimedia-player"))
                .pixmap(96, 96));
        m_artworkLabel->setText(QString());
        return;
    }

    m_artworkLabel->setText(QString());
    m_artworkLabel->setPixmap(
        artwork.scaled(
            m_artworkLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}

QIcon MediaController::playerIcon(const QString &service) const
{
    QString iconName =
        service.mid(QStringLiteral("org.mpris.MediaPlayer2.").length())
            .toLower();
    iconName.replace(
        QRegularExpression(QStringLiteral("[^a-z0-9_-]")),
        QString());

    const QIcon icon = QIcon::fromTheme(iconName);
    return icon.isNull()
        ? QIcon::fromTheme(QStringLiteral("multimedia-player"))
        : icon;
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
