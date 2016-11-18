#include "mbed.h"
#include <stdarg.h>
#include "MTSLog.h"

using namespace mts;

int MTSLog::currentLevel = MTSLog::WARNING_LEVEL;

const char* MTSLog::NONE_LABEL = "NONE";
const char* MTSLog::FATAL_LABEL = "FATAL";
const char* MTSLog::ERROR_LABEL = "ERROR";
const char* MTSLog::WARNING_LABEL = "WARNING";
const char* MTSLog::INFO_LABEL = "INFO";
const char* MTSLog::DEBUG_LABEL = "DEBUG";
const char* MTSLog::TRACE_LABEL = "TRACE";

void MTSLog::printMessage(int level, const char* format, ...) {
    if (printable(level)) {
        va_list argptr;
        va_start(argptr, format);
        vprintf(format, argptr);
        va_end(argptr);
    }
}

bool MTSLog::printable(int level) {
    return level <= currentLevel;
}

void MTSLog::setLogLevel(int level) {
    if (level < NONE_LEVEL)
        currentLevel = NONE_LEVEL;
    else if (level > TRACE_LEVEL)
        currentLevel = TRACE_LEVEL;
    else
    currentLevel = level;
}

int MTSLog::getLogLevel() {
    return currentLevel;
}

const char* MTSLog::getLogLevelString() {
    switch (currentLevel) {
        case NONE_LEVEL:
            return NONE_LABEL;
        case FATAL_LEVEL:
            return FATAL_LABEL;
        case ERROR_LEVEL:
            return ERROR_LABEL;
        case WARNING_LEVEL:
            return WARNING_LABEL;
        case INFO_LEVEL:
            return INFO_LABEL;
        case DEBUG_LEVEL:
            return DEBUG_LABEL;
        case TRACE_LEVEL:
            return TRACE_LABEL;
        default:
            return "unknown";
    }
}
