#include "networking.h"
#include "common.h"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
	size_t total_size = size * nmemb;
	output->append(static_cast<char*>(contents), total_size);
	return total_size;
}

HttpResponse http_get(const std::string& url) {
	CURL* curl = curl_easy_init();
	HttpResponse response;

	if (curl) {
		std::string response_data;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

		CURLcode res = curl_easy_perform(curl);
		response.status_code = res;

		if (res != CURLE_OK) {
			logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, "HTTP GET request failed: " + std::string(curl_easy_strerror(res)));
		}
		else {
			// Parse the JSON response
			response.body = json::parse(response_data);

			if (response.body.is_discarded()) {
				logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, "Error parsing JSON");
			}
		}

		curl_easy_cleanup(curl);
	}
	else {
		logger::Logger::get_instance().log(logger::LogLevel::L_ERROR, "Failed to initialize cURL");
	}
	return response;
}