# M3-A (RRCG, METRO MANILA) catalogue data integrity: every new direction against an independent transcription of
# RRCG's schedule feed, the 15 published pairs pinned to v1.5.0, and label widths with the real fonts.
add_executable(OnPointM3DataTest
  OnPointM3DataTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)
target_include_directories(OnPointM3DataTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src
  ${REPO_ROOT}/lib/EpdFont ${REPO_ROOT}/lib/Utf8)
target_link_libraries(OnPointM3DataTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointM3DataTest)
