# The production OnPointActivity on the host, against the stand-ins in stubs/
# (searched first, so they shadow the device headers it includes).
set(ON_POINT_ACTIVITY_SOURCES
  ${REPO_ROOT}/src/activities/on_point/OnPointActivity.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
)

add_executable(OnPointActivityTest OnPointActivityTest.cpp ${ON_POINT_ACTIVITY_SOURCES}
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp)
add_executable(OnPointActivityEmptyCatalogueTest OnPointActivityEmptyCatalogueTest.cpp ${ON_POINT_ACTIVITY_SOURCES}
  EmptyCatalogue.cpp)
foreach(target OnPointActivityTest OnPointActivityEmptyCatalogueTest)
  target_include_directories(${target} BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs)
  target_include_directories(${target} PRIVATE ${REPO_ROOT}/src)
  target_compile_definitions(${target} PRIVATE ON_POINT_ENABLED=1)
  target_link_libraries(${target} PRIVATE crosspoint_test_common GTest::gtest_main)
  gtest_discover_tests(${target})
endforeach()
