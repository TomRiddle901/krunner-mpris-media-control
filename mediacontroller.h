#pragma once

#include <QMainWindow>

class QComboBox;
class QLabel;
class QPixmap;
class QPushButton;
class QSlider;
class QSystemTrayIcon;
class QTimer;

class MediaController : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.tommaso.MprisMediaController")

public:
    explicit MediaController(QWidget *parent = nullptr);

public Q_SLOTS:
    void toggleVisibility();

protected:
    bool event(QEvent *event) override;

private:
    void refreshPlayers();
    void refreshCurrentPlayer();
    void updatePlayerView();
    void callPlayer(const QString &method);
    void seekTo(int position);
    QString currentService() const;
    QString formatTime(qint64 microseconds) const;
    void showController();
    void updateArtwork(const QVariantMap &data);
    QString technicalMetadata(const QVariantMap &data) const;
    QIcon playerIcon(const QString &service) const;

    QComboBox *m_playerSelector = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_artistLabel = nullptr;
    QLabel *m_technicalLabel = nullptr;
    QLabel *m_artworkLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QSlider *m_positionSlider = nullptr;
    QPushButton *m_previousButton = nullptr;
    QPushButton *m_playPauseButton = nullptr;
    QPushButton *m_nextButton = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QTimer *m_refreshTimer = nullptr;
    bool m_updatingSlider = false;
};
