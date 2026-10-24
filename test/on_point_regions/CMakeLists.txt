# Route collections (Stage 2, P1): the shipped table and the capacity fixture's.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(ON_POINT_REGIONS_MODEL_SOURCES
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
)

add_executable(OnPointRegionsTest OnPointRegionsTest.cpp ${ON_POINT_REGIONS_MODEL_SOURCES}
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp)
target_include_directories(OnPointRegionsTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src)
target_link_libraries(OnPointRegionsTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointRegionsTest)

set(ON_POINT_REGIONS_FIXTURE_CPP ${CMAKE_CURRENT_BINARY_DIR}/OnPointCapacityFixtureData.cpp)
add_custom_command(
  OUTPUT ${ON_POINT_REGIONS_FIXTURE_CPP}
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/test/fixtures/on_point/gen_capacity_fixture.py
    --out ${ON_POINT_REGIONS_FIXTURE_CPP}
  DEPENDS ${REPO_ROOT}/test/fixtures/on_point/gen_capacity_fixture.py
  COMMENT "Generating the ON POINT capacity fixture (collection tests)"
)
add_executable(OnPointRegionsFixtureTest OnPointRegionsFixtureTest.cpp ${ON_POINT_REGIONS_MODEL_SOURCES}
  ${ON_POINT_REGIONS_FIXTURE_CPP})
target_include_directories(OnPointRegionsFixtureTest PRIVATE ${REPO_ROOT}/test/on_point_route_list/stubs ${REPO_ROOT}/src)
target_compile_definitions(OnPointRegionsFixtureTest PRIVATE ON_POINT_CAPACITY_FIXTURE=1)
target_link_libraries(OnPointRegionsFixtureTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointRegionsFixtureTest)
