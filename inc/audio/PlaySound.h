#pragma once

#include <future>
#include <string>
#include <map>
#include <utility>

#ifdef _WIN32
#include "AudioBackend.h"
#endif

struct IStream;

namespace AlsaAudio
{
#ifdef _WIN32
	std::future<int> Play(std::string file_path, std::string device = "default", AudioBackend backend = AudioBackend::WaveOut);
	std::future<int> Play(const void* buf, size_t bufsize, std::string device = "default", AudioBackend backend = AudioBackend::WaveOut);
	std::future<int> Play(IStream* stream, std::string device = "default", AudioBackend backend = AudioBackend::WaveOut);

	std::map<std::string, std::string> ListDevices(AudioBackend backend = AudioBackend::WaveOut);
#else
	std::future<int> Play(std::string file_path, std::string device = "default");
	std::future<int> Play(const void* buf, size_t bufsize, std::string device = "default");
	std::future<int> Play(IStream* stream, std::string device = "default");

	std::map<std::string, std::string> ListDevices();
#endif
}