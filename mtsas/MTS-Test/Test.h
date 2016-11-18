#ifndef TEST_H
#define TEST_H

#include <string>

namespace mts
{

class Test
{
public:
    static void start(std::string testName);
    static void assertTrue(bool condition);
    static void assertFalse(bool condition);
    static void end();
    
    static void clearTotals();
    static int getTotalTests();
    static int getTotalFailed();
    
    static void printPassed(bool print);
    static void printFailed(bool print);

private:
    static std::string testName;
    static bool inProgress;
    
    static int totalTests;
    static int totalFailed;
    static int failed;
    
    static bool pPassed;
    static bool pFailed;
};

}

#endif
