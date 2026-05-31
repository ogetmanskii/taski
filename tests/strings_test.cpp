#include <gtest/gtest.h>
#include "../src/logging/console_logging.hpp"
#include "../src/util/match_wildcard.hpp"

TEST(StringTests, Init) {
	devkit::InitConsole();
}

TEST(StringTests, WildcardMatcherTests) {
    // Базовые тесты
    ASSERT_TRUE(MatchWildcard("qwerty", "q*w*y"));
    ASSERT_TRUE(MatchWildcard(L"qwerty", L"q*w*y"));
    ASSERT_FALSE(MatchWildcard("qwerty", "q*w*z"));
    ASSERT_TRUE(MatchWildcard("qwerty", "qwerty"));
    ASSERT_TRUE(MatchWildcard("", "*"));
    ASSERT_TRUE(MatchWildcard("abc", "*"));

    // Тесты с несколькими звездочками
    ASSERT_TRUE(MatchWildcard("abcdef", "a*c*e*"));
    ASSERT_TRUE(MatchWildcard("abcdef", "a*c*f"));
    ASSERT_TRUE(MatchWildcard("abcdef", "a*b*d*f"));
    ASSERT_TRUE(MatchWildcard("abcdef", "*b*d*"));
    ASSERT_FALSE(MatchWildcard("abcdef", "*b*x*"));

    // Тесты с вопросами
    ASSERT_TRUE(MatchWildcard("abc", "a?c"));
    ASSERT_TRUE(MatchWildcard("abc", "a??"));
    ASSERT_TRUE(MatchWildcard("abc", "???"));
    ASSERT_FALSE(MatchWildcard("abc", "a?b"));

    // Комбинированные тесты
    ASSERT_TRUE(MatchWildcard("abctestxyz", "a*test*"));
    ASSERT_TRUE(MatchWildcard("test123test", "*123*"));
    ASSERT_TRUE(MatchWildcard("test123test", "*123*test"));
    ASSERT_FALSE(MatchWildcard("test123test", "*124*"));

    ASSERT_TRUE(MatchWildcard("bin/test.exe", "*/test.exe"));
    ASSERT_TRUE(MatchWildcard("-H -G 111 -N 50", "*-G 111*"));
}