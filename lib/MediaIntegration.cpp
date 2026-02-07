#include "MediaIntegration.h"
#include <spdlog/spdlog.h>

#if defined(_WIN32)
// Forward declaration — defined in SmtcIntegration.cpp
MediaIntegrationPtr CreateSmtcIntegration(void* hwnd, IMediaCommandCallback* callback);
#elif defined(__linux__)
// Forward declaration — defined in MprisIntegration.cpp
MediaIntegrationPtr CreateMprisIntegration(IMediaCommandCallback* callback);
#elif defined(__APPLE__)
// Forward declaration — defined in NowPlayingIntegration.mm
MediaIntegrationPtr CreateNowPlayingIntegration(IMediaCommandCallback* callback);
#endif

// Fallback no-op implementation for unsupported platforms
class NoOpMediaIntegration : public IMediaIntegration
{
public:
    void UpdateMetadata(const std::string&, const std::string&,
                        const std::string&) noexcept override {}
    void UpdateAlbumArt(const char*, size_t, const std::string&) noexcept override {}
    void UpdatePlaybackStatus(bool) noexcept override {}
    void UpdateProgress(int, int) noexcept override {}
    void SetControlsEnabled(bool) noexcept override {}
};

MediaIntegrationPtr CreateMediaIntegration(void* hwnd, IMediaCommandCallback* callback)
{
#if defined(_WIN32)
    try
    {
        return CreateSmtcIntegration(hwnd, callback);
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to create SMTC integration: {}", e.what());
    }
#elif defined(__linux__)
    (void)hwnd;
    try
    {
        return CreateMprisIntegration(callback);
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to create MPRIS integration: {}", e.what());
    }
#elif defined(__APPLE__)
    (void)hwnd;
    try
    {
        return CreateNowPlayingIntegration(callback);
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to create Now Playing integration: {}", e.what());
    }
#else
    (void)hwnd;
    (void)callback;
#endif
    return std::make_unique<NoOpMediaIntegration>();
}
