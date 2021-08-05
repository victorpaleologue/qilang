#include <gtest/gtest.h>
#include <testqilang/somemix.hpp>

TEST(QiLangConst, constString)
{
  ASSERT_EQ("cette phrase est fausse", testqilang::theOnlyTruth);
}
