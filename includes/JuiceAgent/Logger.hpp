// Logger.hpp
#pragma once
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/DebugOutputAppender.h>

inline void InitLogger()
{
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::DebugOutputAppender<plog::TxtFormatter> debugAppender;

    plog::init(plog::debug, &consoleAppender).addAppender(&debugAppender);

    PLOG_INFO << "Logger initialized: Console + Debug Output";
}