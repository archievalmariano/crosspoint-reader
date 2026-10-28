add_executable(OnPointRouteResolutionTest
  OnPointRouteResolutionTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
)
target_include_directories(OnPointRouteResolutionTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${REPO_ROOT}/src
)
target_compile_definitions(OnPointRouteResolutionTest PRIVATE REPO_ROOT_PATH="${REPO_ROOT}")
target_link_libraries(OnPointRouteResolutionTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointRouteResolutionTest)
