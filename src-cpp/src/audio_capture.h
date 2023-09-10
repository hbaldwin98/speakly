#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <cstddef>
#include <opus/opus_defines.h>

constexpr auto MAX_ENCODED_BUFFER_SIZE = 4096;
constexpr auto BITRATE = OPUS_BITRATE_MAX;
constexpr auto FRAME_SIZE = 480;
constexpr auto SAMPLE_RATE = 48000;
constexpr auto CHANNELS = 1;
#define SIGNAL OPUS_SIGNAL_VOICE
#define APPLICATION OPUS_APPLICATION_VOIP
#define BUFFER_SIZE FRAME_SIZE * CHANNELS 

using EncodedListener = std::function<void(const unsigned char* encoded_data, size_t data_size)>;
using ProcessedListener = std::function<void(const float* encoded_data, size_t data_size)>;
using RawListener = std::function<void(const float* encoded_data, size_t data_size)>;

namespace audio_capture {
	enum class InitializeState {
		NOT_INITIALIZED,
		INITIALIZED,
		PA_ERROR,
		OPUS_ERROR,
		INIT_ERROR
	};

	InitializeState init();
	int decode_output_audio(const std::vector<std::byte>& data);
	int decode_output_audio(unsigned char data[], size_t data_size);
	void get_device_info();
	// Destruct
	void terminate_models();
	void terminate_opus();
	void terminate_portaudio();

	// Attach a listener that can receive encoded audio data
	void attach_encoded_listener(std::shared_ptr<EncodedListener> listener);
	void detach_encoded_listener(std::shared_ptr<EncodedListener> listener);

	// Attach a listener that will receive audio data post-processed.
	void attach_processed_listener(std::shared_ptr<ProcessedListener> listener);
	void detach_processed_listener(std::shared_ptr<ProcessedListener> listener);

	// Attach a listener that will receive raw audio data.
	void attach_raw_listener(std::shared_ptr<RawListener> listener);
	void detach_raw_listener(std::shared_ptr<RawListener> listener);
}