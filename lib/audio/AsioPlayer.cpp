#ifdef _WIN32

#include "LayerCake.h"
#include "audio/AsioPlayer.h"
#include "audio/WaveHeader.h"
extern "C" {
#include <cwASIO.h>
}

#include <spdlog/spdlog.h>

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring>
#include <cassert>
#include <algorithm>

using namespace std;

// ============================================================
// Lock-free Single-Producer Single-Consumer ring buffer
// ============================================================
class SpscRingBuffer
{
public:
	explicit SpscRingBuffer(size_t capacity)
		: m_buffer(capacity)
		, m_capacity(capacity)
		, m_readPos(0)
		, m_writePos(0)
	{
	}

	size_t AvailableRead() const
	{
		const size_t w = m_writePos.load(memory_order_acquire);
		const size_t r = m_readPos.load(memory_order_relaxed);
		return w - r;
	}

	size_t AvailableWrite() const
	{
		const size_t w = m_writePos.load(memory_order_relaxed);
		const size_t r = m_readPos.load(memory_order_acquire);
		return m_capacity - (w - r);
	}

	// Write data into ring buffer. Returns bytes written.
	size_t Write(const void* data, size_t bytes)
	{
		const size_t avail = AvailableWrite();
		const size_t toWrite = (min)(bytes, avail);

		if (toWrite == 0)
			return 0;

		const size_t w = m_writePos.load(memory_order_relaxed);
		const size_t pos = w % m_capacity;

		const size_t firstChunk = (min)(toWrite, m_capacity - pos);
		memcpy(m_buffer.data() + pos, data, firstChunk);

		if (firstChunk < toWrite)
			memcpy(m_buffer.data(), static_cast<const char*>(data) + firstChunk, toWrite - firstChunk);

		m_writePos.store(w + toWrite, memory_order_release);
		return toWrite;
	}

	// Read data from ring buffer. Returns bytes read.
	size_t Read(void* data, size_t bytes)
	{
		const size_t avail = AvailableRead();
		const size_t toRead = (min)(bytes, avail);

		if (toRead == 0)
			return 0;

		const size_t r = m_readPos.load(memory_order_relaxed);
		const size_t pos = r % m_capacity;

		const size_t firstChunk = (min)(toRead, m_capacity - pos);
		memcpy(data, m_buffer.data() + pos, firstChunk);

		if (firstChunk < toRead)
			memcpy(static_cast<char*>(data) + firstChunk, m_buffer.data(), toRead - firstChunk);

		m_readPos.store(r + toRead, memory_order_release);
		return toRead;
	}

private:
	vector<char>	m_buffer;
	size_t			m_capacity;
	atomic<size_t>	m_readPos;
	atomic<size_t>	m_writePos;
};

// ============================================================
// Sample format conversion (Int16 LE input -> ASIO output)
// ============================================================

static void ConvertInt16ToAsio(const int16_t* src, void* dst, long frames, long channels, cwASIOSampleType type)
{
	const long sampleCount = frames * channels;

	switch (type)
	{
	case ASIOSTInt16LSB:
		memcpy(dst, src, sampleCount * sizeof(int16_t));
		break;

	case ASIOSTInt24LSB:
	{
		auto* out = static_cast<uint8_t*>(dst);

		for (long i = 0; i < sampleCount; ++i)
		{
			const int32_t val = static_cast<int32_t>(src[i]) << 8;
			out[i * 3 + 0] = static_cast<uint8_t>(val & 0xFF);
			out[i * 3 + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
			out[i * 3 + 2] = static_cast<uint8_t>((val >> 16) & 0xFF);
		}
		break;
	}

	case ASIOSTInt32LSB:
	case ASIOSTInt32LSB16:
	case ASIOSTInt32LSB18:
	case ASIOSTInt32LSB20:
	case ASIOSTInt32LSB24:
	{
		auto* out = static_cast<int32_t*>(dst);

		for (long i = 0; i < sampleCount; ++i)
			out[i] = static_cast<int32_t>(src[i]) << 16;

		break;
	}

	case ASIOSTFloat32LSB:
	{
		auto* out = static_cast<float*>(dst);

		for (long i = 0; i < sampleCount; ++i)
			out[i] = static_cast<float>(src[i]) / 32768.0f;

		break;
	}

	case ASIOSTFloat64LSB:
	{
		auto* out = static_cast<double*>(dst);

		for (long i = 0; i < sampleCount; ++i)
			out[i] = static_cast<double>(src[i]) / 32768.0;

		break;
	}

	default:
		// Unsupported format - output silence
		spdlog::warn("ASIO: unsupported sample type {}, outputting silence", (int)type);
		memset(dst, 0, frames * channels * 4); // approximate
		break;
	}
}

static size_t GetSampleSize(cwASIOSampleType type)
{
	switch (type)
	{
	case ASIOSTInt16LSB:
	case ASIOSTInt16MSB:
		return 2;
	case ASIOSTInt24LSB:
	case ASIOSTInt24MSB:
		return 3;
	case ASIOSTInt32LSB:
	case ASIOSTInt32MSB:
	case ASIOSTFloat32LSB:
	case ASIOSTFloat32MSB:
	case ASIOSTInt32LSB16:
	case ASIOSTInt32LSB18:
	case ASIOSTInt32LSB20:
	case ASIOSTInt32LSB24:
	case ASIOSTInt32MSB16:
	case ASIOSTInt32MSB18:
	case ASIOSTInt32MSB20:
	case ASIOSTInt32MSB24:
		return 4;
	case ASIOSTFloat64LSB:
	case ASIOSTFloat64MSB:
		return 8;
	default:
		return 4;
	}
}

// ============================================================
// ASIO playback context (global for callbacks)
// ============================================================

struct AsioContext
{
	cwASIODriver*			driver = nullptr;
	SpscRingBuffer*			ringBuffer = nullptr;
	cwASIOBufferInfo			bufferInfos[2] = {};	// stereo: 2 output channels
	long					bufferSize = 0;
	long					numOutputChannels = 0;
	cwASIOSampleType		sampleType = ASIOSTInt16LSB;
	atomic_bool				running{ false };
	atomic_bool				stopRequested{ false };
	mutex					mtx;
	condition_variable		cv;
};

// Only one ASIO driver can be active at a time (ASIO protocol limitation)
static AsioContext* g_asioCtx = nullptr;

static void AsioBufferSwitch(long doubleBufferIndex, cwASIOBool /*directProcess*/)
{
	auto* ctx = g_asioCtx;

	if (!ctx || !ctx->running)
		return;

	const long frames = ctx->bufferSize;
	const size_t sampleSize = GetSampleSize(ctx->sampleType);

	// We have interleaved Int16 stereo in the ring buffer (4 bytes per frame)
	const size_t inputFrameSize = 2 * sizeof(int16_t); // stereo Int16
	const size_t bytesNeeded = frames * inputFrameSize;

	// Temporary buffer for interleaved input
	vector<int16_t> interleavedInput(frames * 2, 0);

	const size_t bytesRead = ctx->ringBuffer->Read(interleavedInput.data(), bytesNeeded);

	if (bytesRead < bytesNeeded)
	{
		// Underrun - pad with silence (already zeroed)
	}

	// ASIO uses separate buffers per channel (de-interleaved)
	for (long ch = 0; ch < ctx->numOutputChannels && ch < 2; ++ch)
	{
		void* outBuf = ctx->bufferInfos[ch].buffers[doubleBufferIndex];

		if (!outBuf)
			continue;

		// De-interleave: extract one channel from stereo interleaved
		vector<int16_t> channelData(frames);

		for (long f = 0; f < frames; ++f)
			channelData[f] = interleavedInput[f * 2 + ch];

		ConvertInt16ToAsio(channelData.data(), outBuf, frames, 1, ctx->sampleType);
	}

	// Signal outputReady if driver supports it
	if (ctx->driver)
		ctx->driver->lpVtbl->outputReady(ctx->driver);

	// Notify feeder thread that space is available
	ctx->cv.notify_one();
}

static void AsioSampleRateChanged(cwASIOSampleRate /*sRate*/)
{
	spdlog::info("ASIO: sample rate changed");
}

static long AsioMessage(long selector, long value, void* /*message*/, double* /*opt*/)
{
	switch (selector)
	{
	case kAsioSelectorSupported:
		switch (value)
		{
		case kAsioEngineVersion:
		case kAsioResetRequest:
		case kAsioSupportsTimeInfo:
			return 1;
		}
		return 0;

	case kAsioEngineVersion:
		return 2;

	case kAsioResetRequest:
		spdlog::info("ASIO: reset request");
		return 1;

	case kAsioLatenciesChanged:
		spdlog::debug("ASIO: latencies changed");
		return 1;

	default:
		return 0;
	}
}

static cwASIOTime* AsioBufferSwitchTimeInfo(cwASIOTime* params, long doubleBufferIndex, cwASIOBool directProcess)
{
	AsioBufferSwitch(doubleBufferIndex, directProcess);
	return params;
}

// ============================================================
// ASIO playback worker
// ============================================================

static int AsioPlayWorker(IStream* stream, const string deviceName)
{
	int result = -1;

	// Read WAV header
	stream->Seek({ 0 }, STREAM_SEEK_SET, nullptr);

	AlsaAudio::WaveHeader wavHeader;
	ULONG bytesRead = 0;

	if (FAILED(stream->Read(&wavHeader, sizeof(wavHeader), &bytesRead)) || bytesRead != sizeof(wavHeader))
	{
		spdlog::error("ASIO: failed to read WAV header");
		return -1;
	}

	if (wavHeader.myData.audioFormat != 1 || wavHeader.myData.numChannels != 2 || wavHeader.myData.bitsPerSample != 16)
	{
		spdlog::error("ASIO: only 16-bit stereo PCM is supported");
		return -1;
	}

	// Find the CLSID for the requested driver
	struct EnumContext
	{
		string targetName;
		string foundId;
	} enumCtx;

	enumCtx.targetName = deviceName;

	cwASIOenumerate([](void* context, char const* name, char const* id, char const* /*description*/) -> bool
		{
			auto* ctx = static_cast<EnumContext*>(context);

			if (name && id)
			{
				if (ctx->targetName == name || ctx->targetName == "default")
				{
					ctx->foundId = id;

					if (ctx->targetName != "default")
						return false; // found exact match
				}
			}
			return true;
		}, &enumCtx);

	if (enumCtx.foundId.empty())
	{
		spdlog::error("ASIO: driver '{}' not found", deviceName);
		return -1;
	}

	// Load driver
	cwASIODriver* driver = nullptr;

	if (cwASIOload(enumCtx.foundId.c_str(), &driver) != 0 || !driver)
	{
		spdlog::error("ASIO: failed to load driver '{}'", deviceName);
		return -1;
	}

	// Initialize driver
	if (!driver->lpVtbl->init(driver, nullptr))
	{
		char errMsg[128] = {};
		driver->lpVtbl->getErrorMessage(driver, errMsg);
		spdlog::error("ASIO: driver init failed: {}", errMsg);
		cwASIOunload(driver);
		return -1;
	}

	{
		char driverName[32] = {};
		driver->lpVtbl->getDriverName(driver, driverName);
		spdlog::info("ASIO: loaded driver '{}' v{}", driverName, driver->lpVtbl->getDriverVersion(driver));
	}

	// Get channel info
	long numInputChannels = 0, numOutputChannels = 0;

	if (driver->lpVtbl->getChannels(driver, &numInputChannels, &numOutputChannels) != ASE_OK || numOutputChannels < 2)
	{
		spdlog::error("ASIO: need at least 2 output channels (have {})", numOutputChannels);
		cwASIOunload(driver);
		return -1;
	}

	// Set sample rate
	if (driver->lpVtbl->setSampleRate(driver, static_cast<double>(wavHeader.myData.sampleRate)) != ASE_OK)
	{
		spdlog::warn("ASIO: failed to set sample rate to {}", wavHeader.myData.sampleRate);
	}

	// Get buffer size
	long minSize = 0, maxSize = 0, preferredSize = 0, granularity = 0;

	if (driver->lpVtbl->getBufferSize(driver, &minSize, &maxSize, &preferredSize, &granularity) != ASE_OK)
	{
		spdlog::error("ASIO: failed to get buffer size");
		cwASIOunload(driver);
		return -1;
	}

	spdlog::info("ASIO: buffer sizes - min={}, max={}, preferred={}, granularity={}", minSize, maxSize, preferredSize, granularity);

	// Get output channel sample type
	cwASIOChannelInfo channelInfo = {};
	channelInfo.channel = 0;
	channelInfo.isInput = ASIOFalse;

	if (driver->lpVtbl->getChannelInfo(driver, &channelInfo) != ASE_OK)
	{
		spdlog::error("ASIO: failed to get channel info");
		cwASIOunload(driver);
		return -1;
	}

	spdlog::info("ASIO: output channel '{}', sample type {}", channelInfo.name, (int)channelInfo.type);

	// Create ring buffer (hold ~200ms worth of audio)
	const size_t inputFrameSize = 2 * sizeof(int16_t); // stereo Int16
	const size_t ringBufferSize = wavHeader.myData.sampleRate * inputFrameSize; // ~1 second
	SpscRingBuffer ringBuffer(ringBufferSize);

	// Set up context
	AsioContext ctx;
	ctx.driver = driver;
	ctx.ringBuffer = &ringBuffer;
	ctx.bufferSize = preferredSize;
	ctx.numOutputChannels = (min)(numOutputChannels, 2L);
	ctx.sampleType = channelInfo.type;
	ctx.running = false;
	ctx.stopRequested = false;

	g_asioCtx = &ctx;

	// Set up callbacks
	cwASIOCallbacks callbacks = {};
	callbacks.bufferSwitch = AsioBufferSwitch;
	callbacks.sampleRateDidChange = AsioSampleRateChanged;
	callbacks.asioMessage = AsioMessage;
	callbacks.bufferSwitchTimeInfo = AsioBufferSwitchTimeInfo;

	// Create buffers for 2 output channels
	cwASIOBufferInfo bufferInfos[2] = {};
	bufferInfos[0].isInput = ASIOFalse;
	bufferInfos[0].channelNum = 0;
	bufferInfos[1].isInput = ASIOFalse;
	bufferInfos[1].channelNum = 1;

	if (driver->lpVtbl->createBuffers(driver, bufferInfos, ctx.numOutputChannels, preferredSize, &callbacks) != ASE_OK)
	{
		spdlog::error("ASIO: failed to create buffers");
		cwASIOunload(driver);
		g_asioCtx = nullptr;
		return -1;
	}

	ctx.bufferInfos[0] = bufferInfos[0];
	ctx.bufferInfos[1] = bufferInfos[1];

	// Pre-fill ring buffer
	{
		const size_t prefillBytes = preferredSize * inputFrameSize * 4; // 4 buffers worth
		vector<char> prefillData(prefillBytes, 0);
		ULONG read = 0;
		ULONG totalRead = 0;

		while (totalRead < prefillBytes)
		{
			if (stream->Read(prefillData.data() + totalRead, static_cast<ULONG>(prefillBytes - totalRead), &read) != S_OK || read == 0)
				break;

			totalRead += read;
		}
		ringBuffer.Write(prefillData.data(), totalRead);
	}

	// Start ASIO
	ctx.running = true;

	if (driver->lpVtbl->start(driver) != ASE_OK)
	{
		spdlog::error("ASIO: failed to start");
		driver->lpVtbl->disposeBuffers(driver);
		cwASIOunload(driver);
		g_asioCtx = nullptr;
		return -1;
	}

	spdlog::info("ASIO: playback started");

	// Feed loop: read from stream, write to ring buffer
	{
		const size_t chunkSize = preferredSize * inputFrameSize;
		vector<char> readBuf(chunkSize);

		while (!ctx.stopRequested)
		{
			// Wait if ring buffer is mostly full
			{
				unique_lock<mutex> lock(ctx.mtx);

				ctx.cv.wait_for(lock, chrono::milliseconds(50), [&]()
					{
						return ringBuffer.AvailableWrite() >= chunkSize || ctx.stopRequested;
					});
			}

			if (ctx.stopRequested)
				break;

			ULONG read = 0;
			ULONG totalRead = 0;
			int nTry = 5;

			while (nTry > 0 && totalRead < chunkSize)
			{
				HRESULT hr = stream->Read(readBuf.data() + totalRead, static_cast<ULONG>(chunkSize - totalRead), &read);

				if (hr != S_OK)
				{
					ctx.stopRequested = true;
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

			if (totalRead > 0)
			{
				size_t written = 0;

				while (written < totalRead && !ctx.stopRequested)
				{
					const size_t w = ringBuffer.Write(readBuf.data() + written, totalRead - written);
					written += w;

					if (w == 0)
					{
						// Ring buffer full, wait for ASIO callback to consume
						unique_lock<mutex> lock(ctx.mtx);
						ctx.cv.wait_for(lock, chrono::milliseconds(20));
					}
				}
			}
		}
	}

	// Stop and cleanup
	ctx.running = false;
	driver->lpVtbl->stop(driver);
	driver->lpVtbl->disposeBuffers(driver);
	cwASIOunload(driver);
	g_asioCtx = nullptr;

	spdlog::info("ASIO: playback stopped");
	result = 0;

	return result;
}

// ============================================================
// Public API
// ============================================================

future<int> AsioPlay(IStream* stream, const string& device)
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
					return AsioPlayWorker(_stream, device);
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

map<string, string> AsioListDevices()
{
	map<string, string> result;

	struct EnumData
	{
		map<string, string>* devices;
	} data;

	data.devices = &result;

	cwASIOenumerate([](void* context, char const* name, char const* id, char const* /*description*/) -> bool
		{
			auto* data = static_cast<EnumData*>(context);

			if (name && id && id[0])
			{
				(*data->devices)[name] = name;
			}
			return true;
		}, &data);

	return result;
}

#endif // _WIN32
