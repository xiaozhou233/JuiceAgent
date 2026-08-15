#pragma once

#include <string>
#include <memory>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Logger {

inline void init(
    const std::string& file_name = "app.log",
    spdlog::level::level_enum level = spdlog::level::trace
) {
    static bool initialized = false;
    if (initialized) {
        spdlog::warn("Logger already initialized, skipping");
        return;
    }
    initialized = true;

    auto console_sink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // truncate = true: overwrite the log file on startup
    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            file_name,
            true
        );

    std::vector<spdlog::sink_ptr> sinks{
        console_sink,
        file_sink
    };

    auto logger =
        std::make_shared<spdlog::logger>(
            "app",
            sinks.begin(),
            sinks.end()
        );

    logger->set_level(level);

    logger->set_pattern(
        "[%Y-%m-%d %H:%M:%S] [%^%l%$] %v"
    );

    spdlog::set_default_logger(logger);
}

}