add_executable(GotoLimitsTest
  GotoLimitsTest.cpp
)
target_include_directories(GotoLimitsTest PRIVATE ${REPO_ROOT}/src)
target_link_libraries(GotoLimitsTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(GotoLimitsTest)
