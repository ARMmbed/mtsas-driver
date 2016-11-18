#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include <vector>

#include "TestCollection.h"
#include "Test.h"

namespace mts
{
class TestRunner
{
public:
    static void addCollection(TestCollection* collection);
    static void clearCollections();
    static void runTests(bool printPassedTests = false, bool printFailedTests = true, bool printPassedCollections = true, bool printFailedCollections = true);

private:
    static std::vector<TestCollection*> collections;
    static int totalCollections;
    static int collectionsFailed;
    static int totalTests;
    static int totalFailed;
};

}

#endif
