add_executable(DogearTest
  DogearTest.cpp
  DogearNetTest.cpp
  DogearCacheTest.cpp
  DogearBootstrapTest.cpp
  DogearStateTest.cpp
)

target_include_directories(DogearTest PRIVATE
  ${REPO_ROOT}/src
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(DogearTest PRIVATE DOGEAR_REPO_ROOT="${REPO_ROOT}")

target_link_libraries(DogearTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(DogearTest)
