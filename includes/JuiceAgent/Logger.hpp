// Logger.hpp
#pragma once
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/DebugOutputAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

inline void InitLogger()
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> colorConsoleAppender;
    static plog::DebugOutputAppender<plog::TxtFormatter> debugAppender;

    plog::init(plog::debug, &colorConsoleAppender).addAppender(&debugAppender);

    PLOG_INFO << "Logger initialized: Console + Debug Output";
}