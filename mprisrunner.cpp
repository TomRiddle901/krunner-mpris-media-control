#include "mprisrunner.h"

#include <KRunner/Action>
#include <KRunner/QueryMatch>
#include <KRunner/RunnerContext>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QEventLoop>
#include <QIcon>
#include <QRegularExpression>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>

namespace {

// Interfaccia MPRIS usata per controllare il player.
constexpr auto kPlayerInterface = "org.mpris.MediaPlayer2.Player";

// Percorso standard dell'interfaccia MPRIS.
constexpr auto kRootPath = "/org/mpris/MediaPlayer2";

struct PlayerInfo {
    // Servizio D-Bus del player.
    QString service;

    // Titolo della canzone.
    QString title;

    // Artista.
    QString artist;

    // Stato del player: Playing / Paused / Stopped...
    QString status;

    // Nome del player, ad esempio Elisa o VLC.
    QString playerName;

    // Posizione corrente del brano in microsecondi.
    qint64 position = 0;

    // Durata totale del brano in microsecondi.
    qint64 duration = 0;
};

// Restituisce il primo elemento di un valore stringa o QStringList.
QString firstString(const QVariant &value)
{
    if (value.canConvert<QStringList>()) {
        const auto list = value.toStringList();

        if (!list.isEmpty())
            return list.first();
    }

    if (value.typeId() == QMetaType::QString)
        return value.toString();

    return {};
}

// Legge i metadata del player MPRIS.
QVariantMap readMetadata(const QDBusInterface &player)
{
    const QVariant value = player.property("Metadata");

    if (value.canConvert<QVariantMap>())
        return value.toMap();

    return {};
}

// Ricava il nome del player dal servizio MPRIS.
QString playerNameFromService(const QString &service)
{
    const QString prefix =
        QStringLiteral("org.mpris.MediaPlayer2.");

    if (!service.startsWith(prefix))
        return {};

    return service.mid(prefix.length());
}

// Restituisce il nome dell'icona del player, se presente nel tema KDE.
QString playerIconName(const PlayerInfo &info)
{
    if (!info.playerName.isEmpty()) {
        QString iconName = info.playerName.toLower();

        // Rimuove caratteri non validi per il nome dell'icona.
        iconName.replace(
            QRegularExpression(QStringLiteral("[^a-z0-9_-]")),
            QString());

        if (!iconName.isEmpty() &&
            QIcon::hasThemeIcon(iconName)) {
            return iconName;
        }
    }

    // Icona generica se quella del player non è disponibile.
    return QStringLiteral("multimedia-player");
}

// Legge le informazioni principali del player.
PlayerInfo readPlayer(const QString &service)
{
    PlayerInfo info;

    // Nome completo del servizio D-Bus.
    info.service = service;

    // Nome del player, ad esempio Elisa o VLC.
    info.playerName = playerNameFromService(service);

    QDBusInterface player(
        service,
        kRootPath,
        kPlayerInterface,
        QDBusConnection::sessionBus());

    if (!player.isValid())
        return info;

    // Stato del player: Playing / Paused / Stopped...
    info.status = player.property("PlaybackStatus").toString();

    // Posizione corrente del brano in microsecondi.
    const QVariant position = player.property("Position");

    if (position.isValid())
        info.position = position.toLongLong();

    const QVariantMap metadata = readMetadata(player);

    // Titolo della canzone.
    info.title = firstString(metadata.value("xesam:title"));

    // Artista della canzone.
    info.artist = firstString(metadata.value("xesam:artist"));

    // Durata del brano in microsecondi.
    const QVariant duration = metadata.value("mpris:length");

    if (duration.isValid())
        info.duration = duration.toLongLong();

    // Testi di fallback nel caso il player non fornisca i metadata.
    if (info.title.isEmpty())
        info.title = QStringLiteral("Sconosciuto");

    if (info.artist.isEmpty())
        info.artist = QStringLiteral("Artista sconosciuto");

    return info;
}

// Cerca tutti i player MPRIS attualmente disponibili.
QStringList findMprisServices()
{
    auto *dbus = QDBusConnection::sessionBus().interface();

    if (!dbus)
        return {};

    const QDBusReply<QStringList> reply =
        dbus->registeredServiceNames();

    if (!reply.isValid())
        return {};

    QStringList services;

    for (const QString &service : reply.value()) {
        // I servizi MPRIS iniziano con questo prefisso.
        if (service.startsWith(
                QStringLiteral("org.mpris.MediaPlayer2."))) {
            services.append(service);
        }
    }

    return services;
}

// Crea il risultato visualizzato da KRunner.
KRunner::QueryMatch createMatch(
    KRunner::AbstractRunner &runner,
    const PlayerInfo &info)
{
    KRunner::QueryMatch match(&runner);

    // Categoria utilizzata dal delegate QML per riconoscere i risultati MPRIS.
    match.setMatchCategory(QStringLiteral("MPRIS"));

    // Titolo principale della canzone.
    match.setText(info.title);

    // Artista mostrato sotto al titolo.
    match.setSubtext(info.artist);

    // Mostra titolo e artista su due righe.
    match.setMultiLine(true);

    // Usa il logo del player invece della copertina.
    match.setIconName(playerIconName(info));

    /*
     * ID del risultato.
     *
     * Oltre al servizio MPRIS salviamo posizione, durata e stato.
     * Il QML utilizza questi dati per costruire la barra di avanzamento.
     */
    match.setId(
        QStringLiteral("mpris:%1|%2|%3|%4")
            .arg(info.service)
            .arg(info.position)
            .arg(info.duration)
            .arg(info.status));

    // Salva il servizio per poter inviare i comandi al player.
    match.setData(info.service);

    // Rende il risultato prioritario.
    match.setRelevance(1.0);

    // Comando per il brano precedente.
    match.addAction(KRunner::Action(
        QStringLiteral("previous"),
        QStringLiteral("media-skip-backward"),
        QStringLiteral("Precedente")));

    // Comando Play/Pausa.
    match.addAction(KRunner::Action(
        QStringLiteral("playpause"),
        info.status == QStringLiteral("Playing")
            ? QStringLiteral("media-playback-pause")
            : QStringLiteral("media-playback-start"),
        info.status == QStringLiteral("Playing")
            ? QStringLiteral("Pausa")
            : QStringLiteral("Riproduci")));

    // Comando per il brano successivo.
    match.addAction(KRunner::Action(
        QStringLiteral("next"),
        QStringLiteral("media-skip-forward"),
        QStringLiteral("Successivo")));

    return match;
}

// Invia un comando MPRIS al player.
void callPlayer(
    const QString &service,
    const QString &method)
{
    QDBusInterface player(
        service,
        kRootPath,
        kPlayerInterface,
        QDBusConnection::sessionBus());

    if (player.isValid())
        player.call(method);
}

} // namespace

MprisRunner::MprisRunner(
    QObject *parent,
    const KPluginMetaData &data)
    : AbstractRunner(parent, data)
{
}

void MprisRunner::match(KRunner::RunnerContext &context)
{
    const QString query = context.query().trimmed();

    // Controlla quale comando è stato digitato.
    const bool isPlayerQuery =
        query.startsWith(
            QStringLiteral("player"),
            Qt::CaseInsensitive);

    const bool isMediaQuery =
        query.startsWith(
            QStringLiteral("media"),
            Qt::CaseInsensitive);

    const bool isMprisQuery =
        query.startsWith(
            QStringLiteral("mpris"),
            Qt::CaseInsensitive);

    if (!isPlayerQuery && !isMediaQuery && !isMprisQuery)
        return;

    const QStringList services = findMprisServices();

    if (services.isEmpty())
        return;

    // "player" mostra un solo player.
    if (isPlayerQuery) {
        PlayerInfo selectedPlayer;
        bool foundPlayer = false;

        // Prima cerca un player in riproduzione.
        for (const QString &service : services) {
            const PlayerInfo info = readPlayer(service);

            if (!info.service.isEmpty() &&
                info.status == QStringLiteral("Playing")) {
                selectedPlayer = info;
                foundPlayer = true;
                break;
            }
        }

        // Se non c'è un player attivo, cerca quello in pausa.
        if (!foundPlayer) {
            for (const QString &service : services) {
                const PlayerInfo info = readPlayer(service);

                if (!info.service.isEmpty() &&
                    info.status == QStringLiteral("Paused")) {
                    selectedPlayer = info;
                    foundPlayer = true;
                    break;
                }
            }
        }

        // Come ultimo fallback usa il primo player disponibile.
        if (!foundPlayer) {
            for (const QString &service : services) {
                const PlayerInfo info = readPlayer(service);

                if (!info.service.isEmpty()) {
                    selectedPlayer = info;
                    foundPlayer = true;
                    break;
                }
            }
        }

        if (!foundPlayer)
            return;

        // Aggiunge una sola card a KRunner.
        context.addMatch(
            createMatch(*this, selectedPlayer));

        return;
    }

    // "media" e "mpris" mostrano tutti i player disponibili.
    QList<KRunner::QueryMatch> matches;

    for (const QString &service : services) {
        const PlayerInfo info = readPlayer(service);

        if (info.service.isEmpty())
            continue;

        matches.append(
            createMatch(*this, info));
    }

    context.addMatches(matches);
}

void MprisRunner::run(
    const KRunner::RunnerContext &context,
    const KRunner::QueryMatch &match)
{
    // Recupera il servizio D-Bus del player selezionato.
    const QString service =
        match.data().toString();

    if (service.isEmpty())
        return;

    // Recupera l'azione selezionata.
    const QString action =
        match.selectedAction().text().isEmpty()
            ? QStringLiteral("playpause")
            : match.selectedAction().id();

    // Ritardo necessario per permettere al player di aggiornare i metadata.
    int refreshDelay = 30;

    // Esegue il comando MPRIS richiesto.
    if (action == QStringLiteral("previous")) {
        callPlayer(
            service,
            QStringLiteral("Previous"));

        refreshDelay = 200;

    } else if (action == QStringLiteral("next")) {
        callPlayer(
            service,
            QStringLiteral("Next"));

        refreshDelay = 200;

    } else {
        callPlayer(
            service,
            QStringLiteral("PlayPause"));
    }

    // Attende l'aggiornamento dei metadata prima di restituire il contesto.
    QEventLoop refreshLoop;
    QTimer::singleShot(
        refreshDelay,
        &refreshLoop,
        &QEventLoop::quit);
    refreshLoop.exec();

    // La richiesta viene letta da KRunner al termine di run(). Cambiare solo
    // lo spazio finale forza il ricalcolo senza chiudere la finestra.
    const QString query = context.query();
    const QString refreshedQuery =
        query.endsWith(QLatin1Char(' '))
            ? query.chopped(1)
            : query + QLatin1Char(' ');

    context.requestQueryStringUpdate(
        refreshedQuery,
        refreshedQuery.length());
}

K_PLUGIN_CLASS_WITH_JSON(
    MprisRunner,
    "mprisrunner.json")

#include "mprisrunner.moc"