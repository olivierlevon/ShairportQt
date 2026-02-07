#ifdef _WIN32

#include "MediaIntegration.h"
#include <spdlog/spdlog.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>
#include <systemmediatransportcontrolsinterop.h>
#include <shcore.h>
#pragma comment(lib, "shcore.lib")

#pragma comment(lib, "windowsapp.lib")

using namespace winrt;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage::Streams;

class SmtcIntegration : public IMediaIntegration
{
public:
    SmtcIntegration(HWND hwnd, IMediaCommandCallback* callback)
        : m_callback(callback)
    {
        auto interop = winrt::get_activation_factory<
            SystemMediaTransportControls,
            ISystemMediaTransportControlsInterop>();

        winrt::check_hresult(interop->GetForWindow(
            hwnd,
            winrt::guid_of<SystemMediaTransportControls>(),
            winrt::put_abi(m_smtc)));

        m_smtc.IsPlayEnabled(true);
        m_smtc.IsPauseEnabled(true);
        m_smtc.IsNextEnabled(true);
        m_smtc.IsPreviousEnabled(true);
        m_smtc.IsStopEnabled(true);
        m_smtc.IsEnabled(true);
        m_smtc.PlaybackStatus(MediaPlaybackStatus::Closed);

        m_buttonPressedToken = m_smtc.ButtonPressed(
            [this](SystemMediaTransportControls const&,
                   SystemMediaTransportControlsButtonPressedEventArgs const& args)
            {
                if (!m_callback) return;

                switch (args.Button())
                {
                case SystemMediaTransportControlsButton::Play:
                case SystemMediaTransportControlsButton::Pause:
                    m_callback->OnMediaCommand("playpause");
                    break;
                case SystemMediaTransportControlsButton::Next:
                    m_callback->OnMediaCommand("nextitem");
                    break;
                case SystemMediaTransportControlsButton::Previous:
                    m_callback->OnMediaCommand("previtem");
                    break;
                case SystemMediaTransportControlsButton::Stop:
                    m_callback->OnMediaCommand("stop");
                    break;
                default:
                    break;
                }
            });

        spdlog::info("SMTC integration initialized");
    }

    ~SmtcIntegration()
    {
        try
        {
            m_smtc.ButtonPressed(m_buttonPressedToken);
            m_smtc.IsEnabled(false);
        }
        catch (...) {}
    }

    void UpdateMetadata(const std::string& artist,
                        const std::string& title,
                        const std::string& album) noexcept override
    {
        try
        {
            auto updater = m_smtc.DisplayUpdater();
            updater.Type(MediaPlaybackType::Music);

            auto props = updater.MusicProperties();
            props.Title(winrt::to_hstring(title));
            props.Artist(winrt::to_hstring(artist));
            props.AlbumTitle(winrt::to_hstring(album));

            updater.Update();
        }
        catch (const winrt::hresult_error& e)
        {
            spdlog::error("SMTC UpdateMetadata failed: {}", winrt::to_string(e.message()));
        }
    }

    void UpdateAlbumArt(const char* data, size_t dataLen,
                        const std::string& imageType) noexcept override
    {
        try
        {
            auto updater = m_smtc.DisplayUpdater();

            if (imageType == "NONE" || dataLen == 0 || !data)
            {
                updater.Thumbnail(nullptr);
            }
            else
            {
                InMemoryRandomAccessStream stream;

                // Write directly via IStream to avoid StoreAsync().get()
                // which asserts on STA threads (Qt main thread is STA)
                winrt::com_ptr<IStream> istream;
                winrt::check_hresult(
                    CreateStreamOverRandomAccessStream(
                        winrt::get_unknown(stream),
                        __uuidof(IStream),
                        istream.put_void()));

                ULONG written = 0;
                winrt::check_hresult(
                    istream->Write(data, static_cast<ULONG>(dataLen), &written));

                stream.Seek(0);

                updater.Thumbnail(
                    RandomAccessStreamReference::CreateFromStream(stream));
            }
            updater.Update();
        }
        catch (const winrt::hresult_error& e)
        {
            spdlog::error("SMTC UpdateAlbumArt failed: {}", winrt::to_string(e.message()));
        }
    }

    void UpdatePlaybackStatus(bool isPlaying) noexcept override
    {
        try
        {
            m_smtc.PlaybackStatus(
                isPlaying ? MediaPlaybackStatus::Playing
                          : MediaPlaybackStatus::Paused);
        }
        catch (...) {}
    }

    void UpdateProgress(int positionSeconds, int durationSeconds) noexcept override
    {
        try
        {
            SystemMediaTransportControlsTimelineProperties timeline;
            timeline.StartTime(std::chrono::seconds{ 0 });
            timeline.EndTime(std::chrono::seconds{ durationSeconds });
            timeline.Position(std::chrono::seconds{ positionSeconds });
            timeline.MinSeekTime(std::chrono::seconds{ 0 });
            timeline.MaxSeekTime(std::chrono::seconds{ durationSeconds });
            m_smtc.UpdateTimelineProperties(timeline);
        }
        catch (...) {}
    }

    void SetControlsEnabled(bool enabled) noexcept override
    {
        try
        {
            m_smtc.IsPlayEnabled(enabled);
            m_smtc.IsPauseEnabled(enabled);
            m_smtc.IsNextEnabled(enabled);
            m_smtc.IsPreviousEnabled(enabled);

            if (!enabled)
            {
                m_smtc.PlaybackStatus(MediaPlaybackStatus::Closed);
            }
        }
        catch (...) {}
    }

private:
    SystemMediaTransportControls m_smtc{ nullptr };
    IMediaCommandCallback* m_callback;
    winrt::event_token m_buttonPressedToken;
};

MediaIntegrationPtr CreateSmtcIntegration(void* hwnd, IMediaCommandCallback* callback)
{
    return std::make_unique<SmtcIntegration>(static_cast<HWND>(hwnd), callback);
}

#endif // _WIN32
