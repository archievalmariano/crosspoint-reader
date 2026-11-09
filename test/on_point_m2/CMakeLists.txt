# M2 (Genesis) catalogue data integrity: every new direction and correction against independent board transcriptions.
add_executable(OnPointM2DataTest
  OnPointM2DataTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
)
target_include_directories(OnPointM2DataTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src)
target_link_libraries(OnPointM2DataTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointM2DataTest)
