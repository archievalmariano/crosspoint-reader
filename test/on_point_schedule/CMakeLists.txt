add_executable(OnPointScheduleTest
  OnPointScheduleTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
)

target_include_directories(OnPointScheduleTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(OnPointScheduleTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(OnPointScheduleTest)
