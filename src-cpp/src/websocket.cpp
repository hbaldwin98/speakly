#include <rtc/rtc.hpp>

#include "websocket.h"
#include "common.h"


namespace websocket {
    std::shared_ptr<rtc::WebSocket> websocket;

    rtc::WebSocket::State get_websocket_state() {
        return websocket->readyState();
    }

    std::shared_ptr<rtc::WebSocket> initialize_websocket(const std::string &uri) {
        websocket = std::make_shared<rtc::WebSocket>();
        websocket->open(uri);

        return websocket;
    }

    void close() {
        if (websocket != nullptr) {
            logger::Logger::get_instance().log(logger::LogLevel::L_INFO, "Closing websocket");
            websocket->close();
        }
    }
}