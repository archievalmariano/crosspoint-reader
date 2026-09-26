add_executable(AppRegistryTest
  AppRegistryTest.cpp
)

target_include_directories(AppRegistryTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(AppRegistryTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(AppRegistryTest)
