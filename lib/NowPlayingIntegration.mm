#ifdef __APPLE__

#include "MediaIntegration.h"
#include <spdlog/spdlog.h>

#import <MediaPlayer/MPNowPlayingInfoCenter.h>
#import <MediaPlayer/MPRemoteCommandCenter.h>
#import <MediaPlayer/MPRemoteCommand.h>
#import <MediaPlayer/MPRemoteCommandEvent.h>
#import <MediaPlayer/MPMediaItemArtwork.h>
#import <AppKit/NSImage.h>
#import <Foundation/Foundation.h>

class NowPlayingIntegration : public IMediaIntegration
{
public:
    NowPlayingIntegration(IMediaCommandCallback* callback)
        : m_callback(callback)
    {
        MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];

        center.playCommand.enabled = YES;
        center.pauseCommand.enabled = YES;
        center.togglePlayPauseCommand.enabled = YES;
        center.nextTrackCommand.enabled = YES;
        center.previousTrackCommand.enabled = YES;
        center.stopCommand.enabled = YES;

        [center.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
            if (m_callback) m_callback->OnMediaCommand("playpause");
            return MPRemoteCommandHandlerStatusSuccess;
        }];

        [center.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
            if (m_callback) m_callback->OnMediaCommand("playpause");
            return MPRemoteCommandHandlerStatusSuccess;
        }];

        [center.togglePlayPauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
            if (m_callback) m_callback->OnMediaCommand("playpause");
            return MPRemoteCommandHandlerStatusSuccess;
        }];

        [center.nextTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
            if (m_callback) m_callback->OnMediaCommand("nextitem");
            return MPRemoteCommandHandlerStatusSuccess;
        }];

        [center.previousTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
            if (m_callback) m_callback->OnMediaCommand("previtem");
            return MPRemoteCommandHandlerStatusSuccess;
        }];

        [center.stopCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
            if (m_callback) m_callback->OnMediaCommand("stop");
            return MPRemoteCommandHandlerStatusSuccess;
        }];

        spdlog::info("macOS Now Playing integration initialized");
    }

    ~NowPlayingIntegration()
    {
        MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];
        [center.playCommand removeTarget:nil];
        [center.pauseCommand removeTarget:nil];
        [center.togglePlayPauseCommand removeTarget:nil];
        [center.nextTrackCommand removeTarget:nil];
        [center.previousTrackCommand removeTarget:nil];
        [center.stopCommand removeTarget:nil];

        MPNowPlayingInfoCenter.defaultCenter.nowPlayingInfo = nil;
    }

    void UpdateMetadata(const std::string& artist,
                        const std::string& title,
                        const std::string& album) noexcept override
    {
        @autoreleasepool
        {
            @try
            {
                m_artist = [NSString stringWithUTF8String:artist.c_str()];
                m_title = [NSString stringWithUTF8String:title.c_str()];
                m_album = [NSString stringWithUTF8String:album.c_str()];
                UpdateNowPlayingInfo();
            }
            @catch (...) {}
        }
    }

    void UpdateAlbumArt(const char* data, size_t dataLen,
                        const std::string& imageType) noexcept override
    {
        @autoreleasepool
        {
            @try
            {
                if (imageType == "NONE" || dataLen == 0 || !data)
                {
                    m_artwork = nil;
                }
                else
                {
                    NSData* imageData = [NSData dataWithBytes:data length:dataLen];
                    NSImage* image = [[NSImage alloc] initWithData:imageData];

                    if (image)
                    {
                        m_artwork = [[MPMediaItemArtwork alloc]
                            initWithBoundsSize:image.size
                            requestHandler:^NSImage* (CGSize size) {
                                return image;
                            }];
                    }
                    else
                    {
                        m_artwork = nil;
                    }
                }
                UpdateNowPlayingInfo();
            }
            @catch (...) {}
        }
    }

    void UpdatePlaybackStatus(bool isPlaying) noexcept override
    {
        @autoreleasepool
        {
            @try
            {
                MPNowPlayingInfoCenter.defaultCenter.playbackState =
                    isPlaying ? MPNowPlayingPlaybackStatePlaying
                              : MPNowPlayingPlaybackStatePaused;
            }
            @catch (...) {}
        }
    }

    void UpdateProgress(int positionSeconds, int durationSeconds) noexcept override
    {
        @autoreleasepool
        {
            @try
            {
                m_duration = @(durationSeconds);
                m_position = @(positionSeconds);
                UpdateNowPlayingInfo();
            }
            @catch (...) {}
        }
    }

    void SetControlsEnabled(bool enabled) noexcept override
    {
        @autoreleasepool
        {
            @try
            {
                MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];
                center.playCommand.enabled = enabled;
                center.pauseCommand.enabled = enabled;
                center.togglePlayPauseCommand.enabled = enabled;
                center.nextTrackCommand.enabled = enabled;
                center.previousTrackCommand.enabled = enabled;

                if (!enabled)
                {
                    MPNowPlayingInfoCenter.defaultCenter.playbackState =
                        MPNowPlayingPlaybackStateStopped;
                }
            }
            @catch (...) {}
        }
    }

private:
    void UpdateNowPlayingInfo()
    {
        NSMutableDictionary* info = [NSMutableDictionary dictionary];

        if (m_title)
            info[MPMediaItemPropertyTitle] = m_title;
        if (m_artist)
            info[MPMediaItemPropertyArtist] = m_artist;
        if (m_album)
            info[MPMediaItemPropertyAlbumTitle] = m_album;
        if (m_artwork)
            info[MPMediaItemPropertyArtwork] = m_artwork;
        if (m_duration)
            info[MPMediaItemPropertyPlaybackDuration] = m_duration;
        if (m_position)
            info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = m_position;

        MPNowPlayingInfoCenter.defaultCenter.nowPlayingInfo = info;
    }

    IMediaCommandCallback* m_callback = nullptr;
    NSString* m_artist = nil;
    NSString* m_title = nil;
    NSString* m_album = nil;
    MPMediaItemArtwork* m_artwork = nil;
    NSNumber* m_duration = nil;
    NSNumber* m_position = nil;
};

MediaIntegrationPtr CreateNowPlayingIntegration(IMediaCommandCallback* callback)
{
    return std::make_unique<NowPlayingIntegration>(callback);
}

#endif // __APPLE__
