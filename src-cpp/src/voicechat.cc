#include <future>
#include <rtc/global.hpp>

#include "voicechat.hpp"
#include "websocket.h"
#include "webrtc.h"
#include "networking.h"
#include "common.h"
#include "audio_capture.h"
#include "logger.h"

auto connect_to_websocket() {
	// Make an HTTP GET request to obtain the JSON token
	HttpResponse response = http_get("http://localhost:9000/connect/token");

	if (response.status_code == CURLE_OK && !response.body.is_discarded()) {
		try {
			if (response.body.contains("token") && response.body["token"].is_string()) {

				std::string uuid = response.body["token"].get<std::string>();
				logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Acquired UUID: " + uuid);
				// Use the acquired UUID when connecting to the WebSocket
				auto websocket = websocket::initialize_websocket("ws:/localhost:9000/connect?token=" + uuid);

				return websocket;
			}
			else {
				logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, "JSON token does not contain 'token' field");
			}

		}
		catch (const json::exception& e) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, std::string("Json parsing error: ") + e.what());
		}
	}

	return std::shared_ptr<rtc::WebSocket>();
}

void begin_connections() {
	auto websocket = connect_to_websocket();

	if (!websocket) {
		logger::Logger::get_instance().log(logger::LogLevel::L_FATAL, "Failed to connect to WebSocket");
		return;
	}

	websocket->onMessage([&](std::variant<rtc::binary, rtc::string> message) {
		if (std::holds_alternative<rtc::string>(message)) {
			try {
				json json_message = json::parse(std::get<std::string>(message));

				if (json_message.is_discarded()) {
					logger::Logger::get_instance().log(logger::LogLevel::L_WARNING, "Invalid Websocket Message: " + std::get<std::string>(message));
					return;
				}

				logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Websocket Message Received: " + json_message.dump());

				if (json_message.contains("type") && json_message.contains("data")) {
					std::string type = json_message["type"].get<std::string>();
					std::string data = json_message["data"].get<std::string>();

					if (type == "sdpAnswer") {
						webrtc::handle_sdp_answer(data);
					}
					else if (type == "iceCandidate") {
						webrtc::handle_ice_candidate(data);
					}
				}
			}
			catch (std::exception& e) {
				logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, std::string("Json parsing error: ") + e.what());
			}
		}
	});

	websocket->onClosed([]() {
		logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Websocket closed");
	});

	std::promise<void> promise;
	std::shared_future<void> websocket_future = promise.get_future().share();

	std::thread([&]() {
		websocket->onOpen([&]() {
			std::cout << "WebSocket opened" << std::endl;
			promise.set_value();
		});
	}).detach();

	websocket_future.get();

	auto pc = webrtc::init_peer_connection(websocket);
	auto dc = webrtc::create_data_channel("myDataChannel");
	auto voip_dc = webrtc::create_data_channel("voip");

	voip_dc->onMessage([](std::variant<rtc::binary, rtc::string> data) {
		if (!std::holds_alternative<rtc::binary>(data)) {
			logger::Logger::get_instance().log(logger::LogLevel::L_WARNING, "Received non-binary data on voip channel");
			return;
		}

		audio_capture::decode_output_audio(std::get<rtc::binary>(data));
	});

	std::promise<void> pc_promise;
	std::shared_future<void> pc_future = pc_promise.get_future().share();

	std::thread([&]() {
		pc->onStateChange([&](rtc::PeerConnection::State state) {
			if (state == rtc::PeerConnection::State::Connected) {
				pc_promise.set_value();
			}
		});
	}).detach();

	pc_future.get();
}

auto voip_listener = std::make_shared<EncodedListener>(
	[](const unsigned char* data, size_t size) {
	// Handle encoded data
	if (webrtc::get_state() == rtc::PeerConnection::State::Connected) {
		webrtc::send_voip_packet(data, size);
	}
});

void init_all() {
	rtc::InitLogger(rtc::LogLevel::Info);
	logger::Logger::get_instance().set_log_file("voicechat.log");

	begin_connections();

	audio_capture::init();
	audio_capture::attach_encoded_listener(voip_listener);
	audio_capture::get_device_info();

	while (true) {
		std::string input;
		std::cin >> input;

		if (input == "exit") {
            break;
		}
	}


	websocket::close();
	webrtc::close();
	audio_capture::terminate_portaudio();
	audio_capture::terminate_opus();
	audio_capture::terminate_models();

	logger::Logger::get_instance().close_log_file();
}