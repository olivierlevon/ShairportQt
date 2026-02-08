#ifdef _WIN32

#include "LayerCake.h"
#include "audio/WasapiPlayer.h"
#include "audio/WaveHeader.h"

#include <Mmdeviceapi.h>
#include <audioclient.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include <avrt.h>

#include <spdlog/spdlog.h>

#include <thread>
#include <vector>
#include <cassert>

#pragma comment(lib, "avrt.lib")

using namespace std;

// Helper: find IMMDevice by endpoint ID string, or return default device
static IMMDevice* GetDevice(IMMDeviceEnumerator* enumerator, const string& deviceId)
{
	IMMDevice* device = nullptr;

	if (deviceId.empty() || deviceId == "default")
	{
		if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)))
		{
			device = nullptr;
		}
	}
	else
	{
		const wstring wideId = CA2WEX(deviceId);

		if (FAILED(enumerator->GetDevice(wideId.c_str(), &device)))
		{
			// fallback to default
			spdlog::warn("WASAPI: device '{}' not found, falling back to default", deviceId);

			if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)))
			{
				device = nullptr;
			}
		}
	}
	return device;
}

// The main WASAPI exclusive playback function (runs on async thread)
static int WasapiPlayWorker(IStream* stream, const string device)
{
	HRESULT hrCoInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	int result = -1;

	IMMDeviceEnumerator* enumerator = nullptr;
	IMMDevice* mmDevice = nullptr;
	IAudioClient* audioClient = nullptr;
	IAudioRenderClient* renderClient = nullptr;
	HANDLE hEvent = nullptr;

	do
	{
		if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), (void**)&enumerator)))
		{
			spdlog::error("WASAPI: failed to create device enumerator");
			break;
		}

		mmDevice = GetDevice(enumerator, device);

		if (!mmDevice)
		{
			spdlog::error("WASAPI: failed to get audio device");
			break;
		}

		if (FAILED(mmDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient)))
		{
			spdlog::error("WASAPI: failed to activate audio client");
			break;
		}

		// Read WAV header from stream
		stream->Seek({ 0 }, STREAM_SEEK_SET, nullptr);

		AlsaAudio::WaveHeader wavHeader;
		ULONG bytesRead = 0;

		if (FAILED(stream->Read(&wavHeader, sizeof(wavHeader), &bytesRead)) || bytesRead != sizeof(wavHeader))
		{
			spdlog::error("WASAPI: failed to read WAV header");
			break;
		}

		if (wavHeader.myData.audioFormat != 1)
		{
			spdlog::error("WASAPI: unsupported audio format (only PCM supported)");
			break;
		}

		// Set up WAVEFORMATEX for exclusive mode
		WAVEFORMATEX wfx = {};
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = wavHeader.myData.numChannels;
		wfx.nSamplesPerSec = wavHeader.myData.sampleRate;
		wfx.wBitsPerSample = wavHeader.myData.bitsPerSample;
		wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		wfx.cbSize = 0;

		// Try exclusive mode
		WAVEFORMATEX* closestMatch = nullptr;
		HRESULT hrFormat = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfx, &closestMatch);

		if (closestMatch)
		{
			CoTaskMemFree(closestMatch);
			closestMatch = nullptr;
		}

		if (hrFormat != S_OK)
		{
			spdlog::warn("WASAPI: exclusive mode format not directly supported (hr=0x{:08x}), trying shared mode fallback", (unsigned)hrFormat);

			// Fall back to shared mode if exclusive is not supported
			hrFormat = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &wfx, &closestMatch);

			if (closestMatch)
			{
				CoTaskMemFree(closestMatch);
				closestMatch = nullptr;
			}

			if (hrFormat != S_OK)
			{
				spdlog::error("WASAPI: audio format not supported");
				break;
			}

			// Use shared mode
			REFERENCE_TIME hnsBufferDuration = 500000; // 50ms

			hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			if (!hEvent)
			{
				spdlog::error("WASAPI: failed to create event");
				break;
			}

			HRESULT hrInit = audioClient->Initialize(
				AUDCLNT_SHAREMODE_SHARED,
				AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				hnsBufferDuration,
				0,
				&wfx,
				nullptr);

			if (FAILED(hrInit))
			{
				spdlog::error("WASAPI: failed to initialize audio client in shared mode (hr=0x{:08x})", (unsigned)hrInit);
				break;
			}
		}
		else
		{
			// Exclusive mode
			REFERENCE_TIME hnsBufferDuration = 100000; // 10ms

			hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			if (!hEvent)
			{
				spdlog::error("WASAPI: failed to create event");
				break;
			}

			HRESULT hrInit = audioClient->Initialize(
				AUDCLNT_SHAREMODE_EXCLUSIVE,
				AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				hnsBufferDuration,
				hnsBufferDuration,
				&wfx,
				nullptr);

			if (hrInit == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
			{
				// Get the aligned buffer size
				UINT32 alignedFrames = 0;
				audioClient->GetBufferSize(&alignedFrames);

				// Re-calculate duration
				hnsBufferDuration = (REFERENCE_TIME)(10000000.0 * alignedFrames / wfx.nSamplesPerSec + 0.5);

				// Release and re-create audio client
				audioClient->Release();
				audioClient = nullptr;

				if (FAILED(mmDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient)))
				{
					spdlog::error("WASAPI: failed to re-activate audio client");
					break;
				}

				hrInit = audioClient->Initialize(
					AUDCLNT_SHAREMODE_EXCLUSIVE,
					AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
					hnsBufferDuration,
					hnsBufferDuration,
					&wfx,
					nullptr);
			}

			if (FAILED(hrInit))
			{
				spdlog::error("WASAPI: failed to initialize audio client (hr=0x{:08x})", (unsigned)hrInit);
				break;
			}
		}

		if (FAILED(audioClient->SetEventHandle(hEvent)))
		{
			spdlog::error("WASAPI: failed to set event handle");
			break;
		}

		UINT32 bufferFrameCount = 0;

		if (FAILED(audioClient->GetBufferSize(&bufferFrameCount)))
		{
			spdlog::error("WASAPI: failed to get buffer size");
			break;
		}

		if (FAILED(audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient)))
		{
			spdlog::error("WASAPI: failed to get render client");
			break;
		}

		spdlog::info("WASAPI: initialized with {} Hz, {}-bit, {} channels, buffer {} frames",
			wfx.nSamplesPerSec, wfx.wBitsPerSample, wfx.nChannels, bufferFrameCount);

		// Boost thread priority for audio
		DWORD taskIndex = 0;
		HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

		// Pre-fill buffer before starting
		{
			BYTE* pData = nullptr;

			if (SUCCEEDED(renderClient->GetBuffer(bufferFrameCount, &pData)))
			{
				const UINT32 bufferBytes = bufferFrameCount * wfx.nBlockAlign;
				ULONG read = 0;

				memset(pData, 0, bufferBytes);
				stream->Read(pData, bufferBytes, &read);
				renderClient->ReleaseBuffer(bufferFrameCount, 0);
			}
		}

		// Start playback
		if (FAILED(audioClient->Start()))
		{
			spdlog::error("WASAPI: failed to start playback");

			if (hTask)
				AvRevertMmThreadCharacteristics(hTask);

			break;
		}

		// Playback loop
		bool playing = true;

		while (playing)
		{
			DWORD waitResult = WaitForSingleObject(hEvent, 2000);

			if (waitResult != WAIT_OBJECT_0)
			{
				spdlog::warn("WASAPI: wait timeout or error");
				break;
			}

			UINT32 numFramesPadding = 0;
			audioClient->GetCurrentPadding(&numFramesPadding);

			UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;

			if (numFramesAvailable == 0)
				continue;

			BYTE* pData = nullptr;

			if (FAILED(renderClient->GetBuffer(numFramesAvailable, &pData)))
				break;

			const UINT32 bytesNeeded = numFramesAvailable * wfx.nBlockAlign;
			ULONG bytesRead2 = 0;

			memset(pData, 0, bytesNeeded);

			int nTry = 5;
			ULONG totalRead = 0;

			while (nTry > 0 && totalRead < bytesNeeded)
			{
				ULONG read = 0;
				HRESULT hr = stream->Read(pData + totalRead, bytesNeeded - totalRead, &read);

				if (hr != S_OK)
				{
					playing = false;
					break;
				}
				if (read == 0)
				{
					nTry--;
					Sleep(5);
				}
				else
				{
					nTry = 5;
					totalRead += read;
				}
			}

			DWORD flags = 0;

			if (totalRead == 0)
				flags = AUDCLNT_BUFFERFLAGS_SILENT;

			renderClient->ReleaseBuffer(numFramesAvailable, flags);
		}

		// Stop and drain
		audioClient->Stop();

		if (hTask)
			AvRevertMmThreadCharacteristics(hTask);

		result = 0;
	}
	while (false);

	// Cleanup
	if (renderClient) renderClient->Release();
	if (audioClient)  audioClient->Release();
	if (mmDevice)     mmDevice->Release();
	if (enumerator)   enumerator->Release();
	if (hEvent)       CloseHandle(hEvent);

	if (SUCCEEDED(hrCoInit))
		CoUninitialize();

	return result;
}

future<int> WasapiPlay(IStream* stream, const string& device)
{
	future<int> result;

	if (stream)
	{
		stream->AddRef();

		try
		{
			result = async(launch::async, [](IStream* stream, const string device) -> int
				{
					SharedPtr<IStream> _stream;
					_stream.Attach(stream);
					return WasapiPlayWorker(_stream, device);
				}, stream, device);
		}
		catch (...)
		{
			stream->Release();
			throw;
		}
	}
	return result;
}

map<string, string> WasapiListDevices()
{
	map<string, string> result;

	IMMDeviceEnumerator* enumerator = nullptr;
	HRESULT hrCoInit = E_FAIL;

	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator), (void**)&enumerator)))
	{
		hrCoInit = CoInitialize(nullptr);

		if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), (void**)&enumerator)))
		{
			if (SUCCEEDED(hrCoInit))
				CoUninitialize();

			return result;
		}
	}

	IMMDeviceCollection* collection = nullptr;

	if (SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)))
	{
		UINT count = 0;
		collection->GetCount(&count);

		for (UINT i = 0; i < count; ++i)
		{
			IMMDevice* endpoint = nullptr;

			if (SUCCEEDED(collection->Item(i, &endpoint)))
			{
				PWSTR pwszId = nullptr;

				if (SUCCEEDED(endpoint->GetId(&pwszId)))
				{
					IPropertyStore* props = nullptr;

					if (SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, &props)))
					{
						PROPVARIANT varName;
						PropVariantInit(&varName);

						if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName)))
						{
							string deviceId = CW2AEX(pwszId);
							string friendlyName = CW2AEX(varName.pwszVal);

							result[move(deviceId)] = move(friendlyName);
							PropVariantClear(&varName);
						}
						props->Release();
					}
					CoTaskMemFree(pwszId);
				}
				endpoint->Release();
			}
		}
		collection->Release();
	}

	enumerator->Release();

	if (SUCCEEDED(hrCoInit))
		CoUninitialize();

	return result;
}

#endif // _WIN32
