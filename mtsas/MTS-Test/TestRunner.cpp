#include "TestRunner.h"

#include "mbed.h"
#include "Test.h"

using namespace mts;

std::vector<TestCollection*> TestRunner::collections;

int TestRunner::totalCollections = 0;
int TestRunner::collectionsFailed = 0;
int TestRunner::totalTests = 0;
int TestRunner::totalFailed = 0;

void TestRunner::addCollection(TestCollection* collection)
{
    collections.push_back(collection);
}

void TestRunner::clearCollections()
{
    collections.clear();
}

void TestRunner::runTests(bool printPassedTests, bool printFailedTests, bool printPassedCollections, bool printFailedCollections)
{
    totalCollections = 0;
    collectionsFailed = 0;
    totalTests = 0;
    totalFailed = 0;
    Test::printPassed(printPassedTests);
    Test::printFailed(printFailedTests);
    for (int i = 0; i < collections.size(); i++) {
        if (printPassedTests || printFailedTests) {
            printf("---Testing Collection: %s---\n\r", collections[i]->getName().c_str());
        }
        collections[i]->run();
        if (Test::getTotalFailed() == 0) {
            if (printPassedCollections) {
                printf("---Collection [%s] - PASSED---\n\r\n\r", collections[i]->getName().c_str());
            }
        } else {
            totalFailed += Test::getTotalFailed();
            collectionsFailed++;
            if (printFailedCollections) {
                printf("---Collection [%s] - FAILED---\n\r\n\r", collections[i]->getName().c_str());
            }
        }
        totalCollections++;
        totalTests += Test::getTotalTests();
        Test::clearTotals();
    }
    printf("\n\r\n\r");
    printf("----------Test Summary----------\n\r");
    printf("[%d/%d] Collections Passed\n\r", (totalCollections - collectionsFailed), totalCollections);
    printf("[%d/%d] Tests Passed\n\r", (totalTests - totalFailed), totalTests);
    printf("Testing: %s\n\r", !totalFailed ? "SUCCESS!" : "FAILURE...");
}
