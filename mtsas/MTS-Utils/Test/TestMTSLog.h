#ifndef TESTMTSLOG_H
#define TESTMTSLOG_H

#include "MTSLog.h"

using namespace mts;

class TestMTSLog : public TestCollection
{
public:
    TestMTSLog();
    ~TestMTSLog();

    virtual void run();
};

TestMTSLog::TestMTSLog() : TestCollection("MTSLog") {}

TestMTSLog::~TestMTSLog() {}

void TestMTSLog::run() {
    Test::start("Setting log level to TRACE: should see messages from all levels");
    MTSLog::setLogLevel(MTSLog::TRACE_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::TRACE_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::TRACE_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();

    Test::start("Setting log level to DEBUG: should see all messages above TRACE");
    MTSLog::setLogLevel(MTSLog::DEBUG_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::DEBUG_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::DEBUG_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();

    Test::start("Setting log level to INFO: should see all messages above DEBUG");
    MTSLog::setLogLevel(MTSLog::INFO_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::INFO_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::INFO_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();

    Test::start("Setting log level to WARNING: should see all messages above INFO");
    MTSLog::setLogLevel(MTSLog::WARNING_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::WARNING_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::WARNING_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();

    Test::start("Setting log level to ERROR: should see all messages above WARNING");
    MTSLog::setLogLevel(MTSLog::ERROR_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::ERROR_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::ERROR_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();

    Test::start("Setting log level to FATAL: should see all messages above ERROR");
    MTSLog::setLogLevel(MTSLog::FATAL_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::FATAL_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::FATAL_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();

    Test::start("Setting log level to NONE: should see no messages");
    MTSLog::setLogLevel(MTSLog::NONE_LEVEL);
    Test::assertTrue(strcmp(MTSLog::getLogLevelString(), MTSLog::NONE_LABEL) == 0);
    Test::assertTrue(MTSLog::getLogLevel() == MTSLog::NONE_LEVEL);
    logFatal();
    logError();
    logWarning();
    logInfo();
    logDebug();
    logTrace();
    Test::end();
}

#endif
