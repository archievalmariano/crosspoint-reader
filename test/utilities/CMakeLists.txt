add_executable(UtilitiesTest
  CalculatorEngineTest.cpp
  CivilCalendarTest.cpp
  MarkedDatesTest.cpp
  ${REPO_ROOT}/src/activities/utilities/calculator/CalculatorEngine.cpp
)

target_include_directories(UtilitiesTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(UtilitiesTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(UtilitiesTest)
