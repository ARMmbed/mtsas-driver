#ifndef TESTMTSTEXT_H
#define TESTMTSTEXT_H

#include <string>
#include <vector>

#include "MTSText.h"

using namespace mts;

class TestMTSText : public TestCollection
{
public:
    TestMTSText();
    ~TestMTSText();

    virtual void run();
};

TestMTSText::TestMTSText() : TestCollection("MTSText") {}

TestMTSText::~TestMTSText() {}

void TestMTSText::run() {
    //Testing split method (char delimeter)
    Test::start("split method (char delimeter)");
    std::string source = "ABC,,456";
    std::vector<std::string> split = Text::split(source, ',');
    Test::assertTrue(split.size() == 3);
    Test::assertTrue(split[0].compare("ABC") == 0);
    Test::assertTrue(split[1].compare("") == 0);
    Test::assertTrue(split[2].compare("456") == 0);
    Test::end();
}

#endif /* TESTMTSTEXT_H */
