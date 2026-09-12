add_executable(QrVersionSelectTest
  QrVersionSelectTest.cpp
  ${REPO_ROOT}/src/util/QrVersionSelect.cpp
)

target_include_directories(QrVersionSelectTest PRIVATE
  ${REPO_ROOT}/src/util
)

target_link_libraries(QrVersionSelectTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(QrVersionSelectTest)
