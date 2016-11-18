#ifndef TESTCOLLECTION_H
#define TESTCOLLECTION_H

#include <string>

namespace mts
{
class TestCollection
{
public:
    TestCollection(std::string testName);
    ~TestCollection();
    std::string getName();
    virtual void run() = 0;

private:
    std::string testName;
};

}

#endif
