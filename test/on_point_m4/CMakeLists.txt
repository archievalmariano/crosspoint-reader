# M4 (LAGUNA) catalogue data integrity: every new direction against an independent transcription of the
# operator boards, Nuvali holidays, the shared calendar scope, Bulacan holidays, the M3-A catalogue, label widths.
add_executable(OnPointM4DataTest
  OnPointM4DataTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)
target_include_directories(OnPointM4DataTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src
  ${REPO_ROOT}/lib/EpdFont ${REPO_ROOT}/lib/Utf8)
target_link_libraries(OnPointM4DataTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointM4DataTest)
