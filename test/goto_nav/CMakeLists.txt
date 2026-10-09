add_executable(GotoNavTest
  GotoNavTest.cpp
)

target_include_directories(GotoNavTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(GotoNavTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(GotoNavTest)
