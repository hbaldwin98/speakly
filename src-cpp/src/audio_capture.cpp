//#include <rnnoise.h>
#include <portaudio.h>
#include <opus/opus.h>

#include "audio_capture.h"
#include "common.h"
#include "plugins/audio_processor.h"
#include "plugins/noise_gate_plugin.h"
#include "plugins/high_pass_plugin.h"

namespace audio_capture {
	PaStream* input_stream;
	PaStream* output_stream;
	OpusEncoder* encoder;
	OpusDecoder* decoder;
//	DenoiseState* rnnoise;
//	RNNModel* rnnoise_model;

	std::shared_ptr<AudioProcessor> audio_processor;

	std::vector<std::shared_ptr<RawListener>> raw_listeners;
	std::vector<std::shared_ptr<RawListener>> processed_listeners;
	std::vector<std::shared_ptr<EncodedListener>> encoded_listeners;

	int encode_audio(float* input_buffer, unsigned char* output_buffer) {
		return opus_encode_float(encoder, input_buffer, FRAME_SIZE, output_buffer, MAX_ENCODED_BUFFER_SIZE);
	}

	int decode_audio(const unsigned char* input_buffer, int input_buffer_size, float* output_buffer) {
		return opus_decode_float(decoder, input_buffer, input_buffer_size, output_buffer, FRAME_SIZE, 0);
	}

	float input_buffer[MAX_ENCODED_BUFFER_SIZE];
	int decode_output_audio(const std::vector<std::byte>& data) {
		if (decoder == nullptr || output_stream == nullptr) {
			return -1;
		}

		auto* data_ptr = reinterpret_cast<const unsigned char*>(data.data());
		size_t data_size = data.size();

		//logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Received audio of size: " + std::to_string(data_size));

		int decoded_data = decode_audio(data_ptr, data_size, input_buffer);

		PaError paError = Pa_WriteStream(output_stream, input_buffer, FRAME_SIZE);
		if (paError != paNoError) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, std::string("Error: Pa_WriteStream() failed with ") + Pa_GetErrorText(paError));
			return -1;
		}

		return 1;
	}

	int decode_output_audio(unsigned char data[], size_t data_size) {
		int decoded_data = decode_audio(data, data_size, input_buffer);

		PaError paError = Pa_WriteStream(output_stream, input_buffer, FRAME_SIZE);
		if (paError != paNoError) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, std::string("Error: Pa_WriteStream() failed with ") + Pa_GetErrorText(paError));
			return -1;
		}

		return 1;
	}

	void compute_audio(float* buffer, long frame_count) {
		unsigned char packet[MAX_ENCODED_BUFFER_SIZE];
		//logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Audio callback called: " + std::to_string(frame_count));

		float frame_in[FRAME_SIZE];
		float frame_out[FRAME_SIZE];
		for (int i = 0; i < frame_count / FRAME_SIZE; ++i) {
			std::memcpy(frame_in, buffer + (i * FRAME_SIZE), FRAME_SIZE * sizeof(float));
            std::memcpy(frame_out, frame_in, FRAME_SIZE * sizeof(float));
			for (const auto& listener : raw_listeners) {
				if (*listener) {
					(*listener)(frame_in, FRAME_SIZE);
				}
			}

//			rnnoise_process_frame(rnnoise, frame_out, frame_in);
			audio_processor->process_audio(frame_out, FRAME_SIZE);

			for (const auto& listener : processed_listeners) {
				if (*listener) {
					(*listener)(frame_out, FRAME_SIZE);
				}
			}

			int packet_size = encode_audio(frame_out, packet);

			for (const auto& listener : encoded_listeners) {
				if (*listener) {
					(*listener)(packet, packet_size);
				}
			}
		}
	}

	PaError pa_stream_callback(const void*in_buffer,
		void* output_buffer,
		unsigned long frame_count,
		const PaStreamCallbackTimeInfo* time_info,
		PaStreamCallbackFlags status_flags,
		void* user_data) {

		float* in = (float*)in_buffer;

		compute_audio(in, frame_count);

		return 0;
	}

	PaError initialize_portaudio() {
		PaError paError;
		paError = Pa_Initialize();

		if (paError != paNoError) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, std::string("Error: Failed to start PortAudio with ") + Pa_GetErrorText(paError));
			return paError;
		}

		paError = Pa_OpenDefaultStream(&input_stream, CHANNELS, 0, paFloat32, SAMPLE_RATE, BUFFER_SIZE, pa_stream_callback, NULL);
		if (paError != paNoError) {
			return paError;
		}

		paError = Pa_OpenDefaultStream(&output_stream, 0, CHANNELS, paFloat32, SAMPLE_RATE, BUFFER_SIZE, NULL, NULL);
		if (paError != paNoError) {
			return paError;
		}

		paError = Pa_StartStream(input_stream);
		if (paError != paNoError) {
			return paError;
		}

		paError = Pa_StartStream(output_stream);
		if (paError != paNoError) {
			return paError;
		}

		return paNoError;
	}

	int initialize_opus() {
		int error;
		encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, APPLICATION, &error);
		decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &error);

		if (error != OPUS_OK) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, std::string("Error: Failed to initialize Opus with ") + opus_strerror(error));
			return error;
		}

		opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));
		opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));
		opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
		opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));
		opus_encoder_ctl(encoder, OPUS_SET_LSB_DEPTH(16));

		return OPUS_OK;
	}

	InitializeState init() {
//		std::string model_path = "./rnnoise_model.rnnn";
//		if (!std::filesystem::exists(model_path)) {
//			std::cerr << "Error: Could not find file " << model_path << std::endl;
//			return InitializeState::INIT_ERROR;
//		}
//
//		FILE* model_file = fopen(model_path.c_str(), "rb");
//		rnnoise_model = rnnoise_model_from_file(model_file);
//
//		rnnoise = rnnoise_create(rnnoise_model);
		audio_processor = std::make_shared<AudioProcessor>();
		audio_processor->add_plugin(std::make_shared<HighPassPlugin>(0.01));
		audio_processor->add_plugin(std::make_shared<NoiseGatePlugin>());

		int opus_state = initialize_opus();

		if (opus_state != OPUS_OK) {
			return InitializeState::OPUS_ERROR;
		}

		PaError pa_state = initialize_portaudio();

		if (pa_state != paNoError) {
			return InitializeState::PA_ERROR;
		}

		return InitializeState::INITIALIZED;
	}

	PaError update_stream_params(const PaStreamParameters* inputParams, const PaStreamParameters* outputParams) {
		PaError paError;

		if (input_stream == NULL) {
			paError = Pa_OpenStream(&input_stream, inputParams, outputParams, SAMPLE_RATE, paFramesPerBufferUnspecified, paClipOff, NULL, NULL);
			if (paError != paNoError) {
				return paError;
			}

			paError = Pa_StartStream(input_stream);
			if (paError != paNoError) {
				return paError;
			}

			return paNoError;
		}

		paError = Pa_StopStream(input_stream);
		if (paError != paNoError) {
			return paError;
		}

		paError = Pa_CloseStream(input_stream);
		if (paError != paNoError) {
			return paError;
		}

		paError = Pa_OpenStream(&input_stream, inputParams, outputParams, SAMPLE_RATE, paFramesPerBufferUnspecified, paClipOff, NULL, NULL);
		if (paError != paNoError) {
			return paError;
		}

		paError = Pa_StartStream(input_stream);
		if (paError != paNoError) {
			return paError;
		}

		return paNoError;
	}


	void get_device_info() {
		for (int i = 0; i < Pa_GetDeviceCount(); i++) {
			const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
			if (device_info->hostApi != Pa_GetDefaultHostApi()) {
				continue;
			}
			if (device_info->maxInputChannels > 0) {
				logger::Logger::get_instance().log(logger::LogLevel::L_INFO, std::string("Input Device: ") + device_info->name + " " + std::to_string(i));
			}
			if (device_info->maxOutputChannels > 0) {
				logger::Logger::get_instance().log(logger::LogLevel::L_INFO, std::string("Output Device: ") + device_info->name + " " + std::to_string(i));
			}
		}
	}

	void attach_raw_listener(std::shared_ptr<RawListener> listener) {
		raw_listeners.push_back(listener);
	}

	void detach_raw_listener(std::shared_ptr<RawListener> listener) {
		auto it = std::remove(raw_listeners.begin(), raw_listeners.end(), listener);
		if (it != raw_listeners.end()) {
			raw_listeners.erase(it);
		}
	}

	void attach_processed_listener(std::shared_ptr<ProcessedListener> listener) {
		processed_listeners.push_back(listener);
	}

	void detach_processed_listener(std::shared_ptr<ProcessedListener> listener) {
		auto it = std::remove(processed_listeners.begin(), processed_listeners.end(), listener);
		if (it != processed_listeners.end()) {
			processed_listeners.erase(it);
		}
	}

	void attach_encoded_listener(std::shared_ptr<EncodedListener> listener) {
		encoded_listeners.push_back(listener);
	}

	void detach_encoded_listener(std::shared_ptr<EncodedListener> listener) {
		auto it = std::remove(encoded_listeners.begin(), encoded_listeners.end(), listener);
		if (it != encoded_listeners.end()) {
			encoded_listeners.erase(it);
		}
	}

	void terminate_models() {
		logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Terminating audio processing services.");
//		rnnoise_destroy(rnnoise);
//		rnnoise_model_free(rnnoise_model);
	}

	void terminate_opus() {
		logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Terminating Opus.");
		opus_encoder_destroy(encoder);
		opus_decoder_destroy(decoder);
	}

	void terminate_portaudio() {
		logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Terminating PortAudio.");
		Pa_StopStream(input_stream);
		Pa_CloseStream(input_stream);
		Pa_Terminate();
	}
}