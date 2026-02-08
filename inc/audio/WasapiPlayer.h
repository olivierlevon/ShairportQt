#pragma once

#ifdef _WIN32

#include <future>
#include <string>
#include <map>

struct IStream;

std::future<int> WasapiPlay(IStream* stream, const std::string& device);
std::map<std::string, std::string> WasapiListDevices();

#endif // _WIN32
