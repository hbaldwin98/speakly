#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <ctime>

namespace logger {
	enum class LogLevel {
		L_INFO,
		L_WARNING,
		L_ERROR,
		L_FATAL
	};

	class Logger {
	public:
		static Logger& get_instance();

		void set_log_file(const std::string& filename);
		void log(LogLevel level, const std::string& message);
		void close_log_file();

	private:
		Logger();
		~Logger();
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		std::string get_timestamp() const;
		std::string get_log_level(LogLevel level) const;

		std::ofstream logFile_;
	};
}
#endif 