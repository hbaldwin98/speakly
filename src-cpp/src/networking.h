#pragma once

#include "curl/curl.h"
#include "nlohmann/json.hpp"

struct HttpResponse {
	CURLcode status_code;
	nlohmann::json body;
};

HttpResponse http_get(const std::string& url);