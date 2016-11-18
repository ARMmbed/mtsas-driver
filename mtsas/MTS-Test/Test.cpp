#include "Test.h"

#include "mbed.h"

using namespace mts;

std::string Test::testName = "";
bool Test::inProgress = false;

int Test::totalTests = 0;
int Test::totalFailed = 0;
int Test::failed = 0;

bool Test::pPassed = true;
bool Test::pFailed = true;

void Test::assertTrue(bool condition)
{
    if (!condition) {
        failed++;
    }
}

void Test::assertFalse(bool condition)
{
    if (condition) {
        failed++;
    }
}

void Test::printPassed(bool print)
{
    pPassed = print;
}

void Test::printFailed(bool print)
{
    pFailed = print;
}

void Test::start(std::string testName)
{
    if (inProgress == true) {
        printf("\n\r\n\r[TEST ERROR] - Tried to start test while another is in progress!\n\r\n\r");
    }
    inProgress = true;
    Test::testName = testName;
    failed = 0;
    totalTests++;
}

void Test::end()
{
    //Check testing state
    if (inProgress == false) {
        printf("\n\r\n\r[TEST ERROR] - Tried to end test that has not been started!\n\r\n\r");
    }

    //Process test
    inProgress = false;
    if (failed == 0) {
        if(pPassed) {
            printf("[%s] - PASSED\n\r", testName.c_str());
        }
    } else {
        totalFailed++;
        if (pFailed) {
            printf("[%s] - FAILED\n\r", testName.c_str());
        }
    }
}

void Test::clearTotals()
{
    totalTests = 0;
    totalFailed = 0;
}

int Test::getTotalTests()
{
    return totalTests;
}

int Test::getTotalFailed()
{
    return totalFailed;
}