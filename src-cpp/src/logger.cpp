#include "logger.h"

namespace logger {
	Logger& Logger::get_instance() {
		static Logger instance;
		return instance;
	}

	void Logger::set_log_file(const std::string& filename) {
		logFile_.open(filename, std::ofstream::out | std::ofstream::app);
	}

	void Logger::log(LogLevel level, const std::string& message) {
		std::string timestamp = get_timestamp();
		std::string levelStr = get_log_level(level);

		std::cout << timestamp << " [" << levelStr << "] " << message << std::endl;

		if (logFile_.is_open()) {
			logFile_ << timestamp << " [" << levelStr << "] " << message << std::endl;
			logFile_.flush();
		}
	}

	void Logger::close_log_file() {
		if (logFile_.is_open()) {
			logFile_.close();
		}
	}

	Logger::Logger() {}

	Logger::~Logger() {
		close_log_file();
	}

	std::string Logger::get_timestamp() const {
		std::time_t now = std::time(nullptr);
		char timestamp[20];
		std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
		return timestamp;
	}

	std::string Logger::get_log_level(LogLevel level) const {
		switch (level) {
		case LogLevel::L_INFO:
			return "INFO";
		case LogLevel::L_WARNING:
			return "WARNING";
		case LogLevel::L_ERROR:
			return "ERROR";
		case LogLevel::L_FATAL:
			return "FATAL";
		default:
			return "UNKNOWN";
		}
	}
}