# Route-list model against the shipped catalogue (golden v1.2.0 behaviour).
add_executable(OnPointRouteListTest
  OnPointRouteListTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
)
target_include_directories(OnPointRouteListTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs  # no-op Logging.h (real one needs Arduino)
  ${REPO_ROOT}/src
)
target_link_libraries(OnPointRouteListTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointRouteListTest)
