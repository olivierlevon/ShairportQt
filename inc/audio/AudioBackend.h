#pragma once

#include <string>

enum class AudioBackend
{
	WaveOut,
	WasapiExclusive,
	Asio
};

inline AudioBackend AudioBackendFromString(const std::string& s)
{
	if (s == "wasapi")
		return AudioBackend::WasapiExclusive;
	if (s == "asio")
		return AudioBackend::Asio;
	return AudioBackend::WaveOut;
}

inline std::string AudioBackendToString(AudioBackend b)
{
	switch (b)
	{
	case AudioBackend::WasapiExclusive:
		return "wasapi";
	case AudioBackend::Asio:
		return "asio";
	default:
		return "waveout";
	}
}
