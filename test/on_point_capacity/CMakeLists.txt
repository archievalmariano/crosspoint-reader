# ON POINT capacity fixture (TEST ONLY): the synthetic 60-route catalogue from
# test/fixtures/on_point/gen_capacity_fixture.py replaces OnPointScheduleData.cpp.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(ON_POINT_FIXTURE_GEN ${REPO_ROOT}/test/fixtures/on_point/gen_capacity_fixture.py)
set(ON_POINT_FIXTURE_CPP ${CMAKE_CURRENT_BINARY_DIR}/OnPointCapacityFixtureData.cpp)
add_custom_command(
  OUTPUT ${ON_POINT_FIXTURE_CPP}
  COMMAND ${Python3_EXECUTABLE} ${ON_POINT_FIXTURE_GEN} --out ${ON_POINT_FIXTURE_CPP}
  DEPENDS ${ON_POINT_FIXTURE_GEN}
  COMMENT "Generating the ON POINT capacity fixture"
)

set(ON_POINT_MODEL_SOURCES
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
)

function(on_point_target name)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs ${REPO_ROOT}/src)
  target_link_libraries(${name} PRIVATE crosspoint_test_common)
endfunction()

# Fixture tests (ctest).
add_executable(OnPointCapacityFixtureTest OnPointCapacityFixtureTest.cpp ${ON_POINT_FIXTURE_CPP} ${ON_POINT_MODEL_SOURCES})
on_point_target(OnPointCapacityFixtureTest)
target_compile_definitions(OnPointCapacityFixtureTest PRIVATE ON_POINT_CAPACITY_FIXTURE=1 ON_POINT_COUNT_COMPARISONS=1)
target_link_libraries(OnPointCapacityFixtureTest PRIVATE GTest::gtest_main)
gtest_discover_tests(OnPointCapacityFixtureTest)

# Benchmarks (not ctest): the same harness over the shipped 7 routes and the 60-route fixture.
add_executable(OnPointRouteListBench7 OnPointRouteListBench.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp ${ON_POINT_MODEL_SOURCES})
on_point_target(OnPointRouteListBench7)
target_compile_definitions(OnPointRouteListBench7 PRIVATE ON_POINT_COUNT_COMPARISONS=1 BENCH_LABEL="shipped-7")

add_executable(OnPointRouteListBench60 OnPointRouteListBench.cpp ${ON_POINT_FIXTURE_CPP} ${ON_POINT_MODEL_SOURCES})
on_point_target(OnPointRouteListBench60)
target_compile_definitions(OnPointRouteListBench60 PRIVATE ON_POINT_CAPACITY_FIXTURE=1 ON_POINT_COUNT_COMPARISONS=1
  BENCH_LABEL="fixture-60")

# Release-exclusion guard over platformio.ini (ctest).
add_test(NAME OnPointCapacityFixtureExcludedFromReleases
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/check_no_capacity_fixture.py --config ${REPO_ROOT}/platformio.ini
    --workflow ${REPO_ROOT}/.github/workflows/release_candidate.yml)
add_test(NAME OnPointCapacityFixtureGuardTests
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/scripts/test_check_no_capacity_fixture.py)
