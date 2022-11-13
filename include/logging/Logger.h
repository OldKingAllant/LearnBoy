#pragma once

#include <format>
#include <string_view>
#include <iostream>

/*
The macros log the file and the line
*/

#define LOG_INFO(logger, fmt, ...) \
    logger.log_info("I {0}:{1} " fmt, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_WARN(logger, fmt, ...) \
    logger.log_warn("W {0}:{1} " fmt, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_ERR(logger, fmt, ...) \
    logger.log_err("E {0}:{1} " fmt, __FILE__, __LINE__, __VA_ARGS__)

class Logger {
public :
	//to implement
	//simply prints directly to console, no buffering
	/*
	* Maybe add :
	* - Colored output
	* - Redirect to file
	* - Add timestamp
	* - Add more options to the logging
	* ...
	*/
	template <typename... Args>
	void log_info(std::string_view fmt,
		Args&&... args) {
		std::cout << std::vformat(
			fmt, std::make_format_args(
				std::forward<Args>(args)...
			)
		);
	}

	template <typename... Args>
	void log_warn(std::string_view fmt,
		Args&&... args) {
		std::cout << std::vformat(
			fmt, std::make_format_args(
				std::forward<Args>(args)...
			)
		);
	}

	template <typename... Args>
	void log_err(std::string_view fmt,
		Args&&... args) {
		std::cout << std::vformat(
			fmt, std::make_format_args(
				std::forward<Args>(args)...
			)
		);
	}

	void flush() {
		std::cout.flush();
	}
};