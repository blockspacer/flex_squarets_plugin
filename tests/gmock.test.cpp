#include "testsCommon.h"

#if !defined(USE_GTEST_TEST)
#warning "use USE_GTEST_TEST"
// default
#define USE_GTEST_TEST 1
#endif // !defined(USE_GTEST_TEST)

TEST(dummyTest, LinksFine) {
  // We just check that code can compile and link
  EXPECT_TRUE(true);
}
