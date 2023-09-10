#pragma once

#include <memory>
#include <rtc/peerconnection.hpp>
#include <rtc/websocket.hpp>

namespace webrtc {
	// Initialize the WebRTC peer connection. Requires a websocket to send the local description
	// once it has been established.
	std::shared_ptr<rtc::PeerConnection> init_peer_connection(const std::weak_ptr <rtc::WebSocket>& websocket);

	rtc::Configuration get_config();

	rtc::PeerConnection::State get_state();

	void handle_sdp_answer(const std::string& data);

	void handle_ice_candidate(const std::string& data);

	std::shared_ptr<rtc::DataChannel> create_data_channel(const std::string& label);

	void send_network_packet(const std::weak_ptr<rtc::DataChannel>& data_channel, const unsigned char* packet, int current_packet_size);

	void send_voip_packet(const unsigned char* packet, int current_packet_size);

	void close();
}