#include "TestCollection.h"

using namespace mts;

TestCollection::TestCollection(std::string testName) : testName(testName)
{
}

TestCollection::~TestCollection()
{
}

std::string TestCollection::getName()
{
    return testName;
}