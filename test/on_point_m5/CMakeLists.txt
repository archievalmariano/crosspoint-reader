# M5 (SOUTH LUZON) catalogue data integrity: every new direction against an independent transcription of the
# RRCG boards, day rules, collection membership, the M4 catalogue, label widths.
add_executable(OnPointM5DataTest
  OnPointM5DataTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)
target_include_directories(OnPointM5DataTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src
  ${REPO_ROOT}/lib/EpdFont ${REPO_ROOT}/lib/Utf8)
target_link_libraries(OnPointM5DataTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointM5DataTest)
