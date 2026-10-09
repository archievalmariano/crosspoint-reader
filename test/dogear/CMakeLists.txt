add_executable(DogearTest
  DogearTest.cpp
  DogearNetTest.cpp
  DogearCacheTest.cpp
)

target_include_directories(DogearTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(DogearTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(DogearTest)
