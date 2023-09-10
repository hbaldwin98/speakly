#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <rtc/rtc.hpp>

using json = nlohmann::json;

struct WebSocketMessage {
	std::string type;
	std::string data;

	[[nodiscard]] json toJson() const {
		json jsonObj;
		jsonObj["type"] = type;
		jsonObj["data"] = data;
		return jsonObj;
	}
};

namespace websocket {
	std::shared_ptr<rtc::WebSocket> initialize_websocket(const std::string& uri);
	void close();
};
