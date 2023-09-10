#include "webrtc.h"
#include "websocket.h"
#include "common.h"

namespace webrtc {
	rtc::Configuration config;
	std::shared_ptr<rtc::PeerConnection> pc;
	std::unordered_map <std::string, std::shared_ptr<rtc::DataChannel>> data_channels;

	rtc::Configuration get_config() {
		return config;
	}

	rtc::PeerConnection::State get_state() {
		return pc->state();
	}

	void send_network_packet(const std::weak_ptr<rtc::DataChannel>& data_channel, const unsigned char* packet, int current_packet_size) {
		if (auto dc = data_channel.lock()) {
			std::vector<std::byte> byte_packet(current_packet_size);

			for (int i = 0; i < current_packet_size; i++) {
				byte_packet[i] = static_cast<std::byte>(packet[i]);
			}

			// Send the byte packet
			dc->send(byte_packet.data(), byte_packet.size());
		}
	}

	void send_voip_packet(const unsigned char* packet, int current_packet_size) {
		if (auto dc = data_channels["voip"]) {
			if (!dc->isOpen()) {
				return;
			}

			std::vector<std::byte> byte_packet(current_packet_size);

			for (int i = 0; i < current_packet_size; i++) {
				byte_packet[i] = static_cast<std::byte>(packet[i]);
			}

			// Send the byte packet
			dc->send(byte_packet.data(), byte_packet.size());
		}
	}


	void handle_sdp_answer(const std::string& data) {
		pc->setRemoteDescription(data);
	}

	void handle_ice_candidate(const std::string& data) {
		try {
			json json_data = json::parse(data);

			if (!json_data.contains("candidate")) {
				return;
			}

			rtc::Candidate candidate(json_data["candidate"].get<std::string>());

			pc->addRemoteCandidate(candidate);

		}
		catch (const json::parse_error& e) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, "Error parsing JSON: " + std::string(e.what()));
		}
	}

	std::shared_ptr<rtc::DataChannel> create_data_channel(const std::string& label) {
		if (pc == nullptr || pc->state() == rtc::PeerConnection::State::Failed || pc->state() == rtc::PeerConnection::State::Disconnected) {
			return nullptr;
		}

		if (data_channels.find(label) != data_channels.end()) {
			return data_channels[label];
		}

		logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Creating data channel: " + label);
		auto data_channel = pc->createDataChannel(label);
		if (data_channel == nullptr) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, "Error creating data channel: " + label);
			return nullptr;
		}

		data_channels[label] = data_channel;
		return data_channel;
	}

	std::shared_ptr<rtc::PeerConnection> init_peer_connection(const std::weak_ptr <rtc::WebSocket>& websocket) {
		//config.iceServers.emplace_back("stun:stun.l.google.com:19302");
		pc = std::make_shared<rtc::PeerConnection>(config);
		logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Created peer connection");

		pc->onLocalDescription([websocket](const rtc::Description& sdp) {
			logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Received local description");

			WebSocketMessage message;
			message.type = "sdpOffer";
			message.data = sdp.generateSdp();

			// Convert the message to JSON using nlohmann/json
			json jsonMessage = message.toJson();
			std::string jsonString = jsonMessage.dump();

			if (auto ws = websocket.lock()) {
				ws->send(jsonString);
			}
		});


		pc->onLocalCandidate([websocket](const rtc::Candidate& candidate) {
			logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Received local ice candidate");
			if (auto ws = websocket.lock()) {
				//ws->send(candidate.candidate(), candidate.mid());
			}
		});

		pc->onStateChange([](rtc::PeerConnection::State state) {
			std::string state_str;

			switch (state) {
			case rtc::PeerConnection::State::New:
				state_str = "New";
				break;
			case rtc::PeerConnection::State::Connecting:
				state_str = "Connecting";
				break;
			case rtc::PeerConnection::State::Connected:
				state_str = "Connected";
				break;
			case rtc::PeerConnection::State::Disconnected:
				state_str = "Disconnected";
				break;
			case rtc::PeerConnection::State::Failed:
				state_str = "Failed";
				break;
			case rtc::PeerConnection::State::Closed:
				state_str = "Closed";
				break;
			default:
				state_str = "Unknown";
				break;
			}

			logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "WebRTC State: " + state_str);
		});

		pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
			std::string state_str;
			switch (state) {
			case rtc::PeerConnection::GatheringState::New:
				state_str = "New";
				break;
			case rtc::PeerConnection::GatheringState::InProgress:
				state_str = "InProgress";
				break;
			case rtc::PeerConnection::GatheringState::Complete:
				state_str = "Complete";
				break;
			default:
				state_str = "Unknown";
				break;
			}

			logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "WebRTC Gathering State: " + state_str);
		});

		pc->onDataChannel([](const std::shared_ptr<rtc::DataChannel>& dc) {
			dc->onOpen([dc]() {
				logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Data channel open " + std::to_string(dc->id().value()));
			});

			dc->onClosed([dc]() {
				logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Data channel closed " + std::to_string(dc->id().value()));
			});

			/*	dc->onMessage([dc](auto data) {
					std::cout << "message from data channel " << dc->id().value() << std::endl;
				});*/
		});

		return pc;
	}

	void close() {
		if (pc != nullptr) {
			for (auto& dc : data_channels) {
				if (dc.second != nullptr) {
					logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Closing data channel: " + dc.first);
					dc.second->close();
				}
			}

			logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Closing peer connection");
			pc->close();
		}
	}
}