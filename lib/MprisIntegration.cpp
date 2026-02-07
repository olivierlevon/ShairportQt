#ifdef __linux__

#include "MediaIntegration.h"
#include <spdlog/spdlog.h>

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QVariantMap>
#include <QStringList>
#include <QFile>
#include <QDir>

// Root QObject registered at /org/mpris/MediaPlayer2
class MprisObject : public QObject
{
    Q_OBJECT
public:
    explicit MprisObject(QObject* parent = nullptr) : QObject(parent) {}
};

// org.mpris.MediaPlayer2 interface
class MediaPlayer2Adaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")

    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)

public:
    explicit MediaPlayer2Adaptor(QObject* parent)
        : QDBusAbstractAdaptor(parent) {}

    bool canQuit() const { return false; }
    bool canRaise() const { return false; }
    bool hasTrackList() const { return false; }
    QString identity() const { return QStringLiteral("ShairportQt"); }
    QStringList supportedMimeTypes() const { return {}; }
    QStringList supportedUriSchemes() const { return {}; }

public slots:
    void Quit() {}
    void Raise() {}
};

// org.mpris.MediaPlayer2.Player interface
class PlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")

    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanControl READ canControl)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(qlonglong Position READ position)

public:
    explicit PlayerAdaptor(QObject* parent, IMediaCommandCallback* callback)
        : QDBusAbstractAdaptor(parent)
        , m_callback(callback)
    {
        m_artFilePath = QDir::tempPath() + "/shairportqt_albumart";
    }

    QString playbackStatus() const { return m_playbackStatus; }
    QVariantMap metadata() const { return m_metadata; }
    bool canPlay() const { return m_controlsEnabled; }
    bool canPause() const { return m_controlsEnabled; }
    bool canGoNext() const { return m_controlsEnabled; }
    bool canGoPrevious() const { return m_controlsEnabled; }
    bool canControl() const { return m_controlsEnabled; }
    bool canSeek() const { return false; }
    qlonglong position() const { return m_positionUs; }

    void setPlaybackStatus(const QString& status)
    {
        if (m_playbackStatus != status)
        {
            m_playbackStatus = status;
            emitPropertyChanged("PlaybackStatus", status);
        }
    }

    void setMetadata(const QString& artist, const QString& title,
                     const QString& album, const QString& artUrl)
    {
        QVariantMap meta;
        meta["mpris:trackid"] = QVariant::fromValue(
            QDBusObjectPath("/org/mpris/MediaPlayer2/CurrentTrack"));

        if (!title.isEmpty())
            meta["xesam:title"] = title;
        if (!artist.isEmpty())
            meta["xesam:artist"] = QStringList{ artist };
        if (!album.isEmpty())
            meta["xesam:album"] = album;
        if (!artUrl.isEmpty())
            meta["mpris:artUrl"] = artUrl;
        if (m_durationUs > 0)
            meta["mpris:length"] = m_durationUs;

        m_metadata = meta;
        emitPropertyChanged("Metadata", QVariant::fromValue(meta));
    }

    void setControlsEnabled(bool enabled)
    {
        if (m_controlsEnabled != enabled)
        {
            m_controlsEnabled = enabled;

            QVariantMap changed;
            changed["CanPlay"] = enabled;
            changed["CanPause"] = enabled;
            changed["CanGoNext"] = enabled;
            changed["CanGoPrevious"] = enabled;
            changed["CanControl"] = enabled;
            emitPropertiesChanged(changed);
        }
    }

    void setPosition(qlonglong positionUs, qlonglong durationUs)
    {
        m_positionUs = positionUs;
        m_durationUs = durationUs;
    }

    QString writeAlbumArt(const char* data, size_t dataLen,
                          const std::string& imageType)
    {
        if (imageType == "NONE" || dataLen == 0 || !data)
        {
            QFile::remove(m_artFilePath + ".jpg");
            QFile::remove(m_artFilePath + ".png");
            return {};
        }

        QString ext = ".jpg";
        if (imageType == "image/png" || imageType == "PNG")
            ext = ".png";

        QString filePath = m_artFilePath + ext;
        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file.write(data, static_cast<qint64>(dataLen));
            file.close();
            return "file://" + filePath;
        }
        return {};
    }

public slots:
    void Play()      { if (m_callback) m_callback->OnMediaCommand("playpause"); }
    void Pause()     { if (m_callback) m_callback->OnMediaCommand("playpause"); }
    void PlayPause() { if (m_callback) m_callback->OnMediaCommand("playpause"); }
    void Next()      { if (m_callback) m_callback->OnMediaCommand("nextitem"); }
    void Previous()  { if (m_callback) m_callback->OnMediaCommand("previtem"); }
    void Stop()      { if (m_callback) m_callback->OnMediaCommand("stop"); }

private:
    void emitPropertyChanged(const QString& property, const QVariant& value)
    {
        QVariantMap changed;
        changed[property] = value;
        emitPropertiesChanged(changed);
    }

    void emitPropertiesChanged(const QVariantMap& changed)
    {
        QDBusMessage signal = QDBusMessage::createSignal(
            "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged");

        signal << "org.mpris.MediaPlayer2.Player";
        signal << changed;
        signal << QStringList{};

        QDBusConnection::sessionBus().send(signal);
    }

    IMediaCommandCallback* m_callback = nullptr;
    QString m_playbackStatus = "Stopped";
    QVariantMap m_metadata;
    bool m_controlsEnabled = false;
    qlonglong m_positionUs = 0;
    qlonglong m_durationUs = 0;
    QString m_artFilePath;
};

class MprisIntegration : public IMediaIntegration
{
public:
    MprisIntegration(IMediaCommandCallback* callback)
    {
        m_object = std::make_unique<MprisObject>();

        new MediaPlayer2Adaptor(m_object.get());
        m_playerAdaptor = new PlayerAdaptor(m_object.get(), callback);

        auto bus = QDBusConnection::sessionBus();

        if (!bus.registerService("org.mpris.MediaPlayer2.ShairportQt"))
        {
            spdlog::error("MPRIS: failed to register service: {}",
                bus.lastError().message().toStdString());
        }
        if (!bus.registerObject("/org/mpris/MediaPlayer2", m_object.get(),
                QDBusConnection::ExportAdaptors))
        {
            spdlog::error("MPRIS: failed to register object: {}",
                bus.lastError().message().toStdString());
        }
        spdlog::info("MPRIS integration initialized");
    }

    ~MprisIntegration()
    {
        auto bus = QDBusConnection::sessionBus();
        bus.unregisterObject("/org/mpris/MediaPlayer2");
        bus.unregisterService("org.mpris.MediaPlayer2.ShairportQt");
    }

    void UpdateMetadata(const std::string& artist,
                        const std::string& title,
                        const std::string& album) noexcept override
    {
        try
        {
            m_currentArtist = QString::fromStdString(artist);
            m_currentTitle = QString::fromStdString(title);
            m_currentAlbum = QString::fromStdString(album);

            m_playerAdaptor->setMetadata(
                m_currentArtist, m_currentTitle,
                m_currentAlbum, m_currentArtUrl);
        }
        catch (...) {}
    }

    void UpdateAlbumArt(const char* data, size_t dataLen,
                        const std::string& imageType) noexcept override
    {
        try
        {
            m_currentArtUrl = m_playerAdaptor->writeAlbumArt(data, dataLen, imageType);

            m_playerAdaptor->setMetadata(
                m_currentArtist, m_currentTitle,
                m_currentAlbum, m_currentArtUrl);
        }
        catch (...) {}
    }

    void UpdatePlaybackStatus(bool isPlaying) noexcept override
    {
        try
        {
            m_playerAdaptor->setPlaybackStatus(isPlaying ? "Playing" : "Paused");
        }
        catch (...) {}
    }

    void UpdateProgress(int positionSeconds, int durationSeconds) noexcept override
    {
        try
        {
            m_playerAdaptor->setPosition(
                static_cast<qlonglong>(positionSeconds) * 1000000LL,
                static_cast<qlonglong>(durationSeconds) * 1000000LL);
        }
        catch (...) {}
    }

    void SetControlsEnabled(bool enabled) noexcept override
    {
        try
        {
            m_playerAdaptor->setControlsEnabled(enabled);

            if (!enabled)
            {
                m_playerAdaptor->setPlaybackStatus("Stopped");
            }
        }
        catch (...) {}
    }

private:
    std::unique_ptr<MprisObject> m_object;
    PlayerAdaptor* m_playerAdaptor = nullptr;
    QString m_currentArtist;
    QString m_currentTitle;
    QString m_currentAlbum;
    QString m_currentArtUrl;
};

MediaIntegrationPtr CreateMprisIntegration(IMediaCommandCallback* callback)
{
    return std::make_unique<MprisIntegration>(callback);
}

#include "MprisIntegration.moc"

#endif // __linux__
