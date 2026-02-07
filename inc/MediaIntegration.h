#pragma once

#include <string>
#include <memory>
#include <stdint.h>

// Callback interface for media transport commands.
// Commands use the same strings as SendDacpCommand:
// "playpause", "nextitem", "previtem", "stop"
class IMediaCommandCallback
{
public:
    virtual ~IMediaCommandCallback() = default;
    virtual void OnMediaCommand(const std::string& command) noexcept = 0;
};

// Abstract media integration interface (SMTC on Windows, MPRIS on Linux)
class IMediaIntegration
{
public:
    virtual ~IMediaIntegration() = default;

    // Update track metadata (called from Qt main thread)
    virtual void UpdateMetadata(const std::string& artist,
                                const std::string& title,
                                const std::string& album) noexcept = 0;

    // Update album art (raw image bytes + format "JPEG"/"PNG", or empty data + "NONE" to clear)
    virtual void UpdateAlbumArt(const char* data, size_t dataLen,
                                const std::string& imageType) noexcept = 0;

    // Update playback status
    virtual void UpdatePlaybackStatus(bool isPlaying) noexcept = 0;

    // Update progress
    virtual void UpdateProgress(int positionSeconds, int durationSeconds) noexcept = 0;

    // Enable/disable transport controls (when DACP service is available/unavailable)
    virtual void SetControlsEnabled(bool enabled) noexcept = 0;
};

using MediaIntegrationPtr = std::unique_ptr<IMediaIntegration>;

// Factory: creates the platform-appropriate implementation.
// hwnd: on Windows, the HWND of the main window (for SMTC interop). On Linux, pass nullptr.
// callback: receives play/pause/next/prev commands from the OS media controls.
MediaIntegrationPtr CreateMediaIntegration(void* hwnd, IMediaCommandCallback* callback);
