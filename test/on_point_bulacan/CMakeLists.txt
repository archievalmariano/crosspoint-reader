# BULACAN catalogue data integrity: every directional timetable against an
# independent transcription of the editorial tables (BULACAN-BATCH1.md).
add_executable(OnPointBulacanDataTest
  OnPointBulacanDataTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
)
target_include_directories(OnPointBulacanDataTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src)
target_link_libraries(OnPointBulacanDataTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointBulacanDataTest)
