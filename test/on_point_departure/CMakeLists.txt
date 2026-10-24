# getDepartureState vs the frozen reference (test/fixtures/on_point/LegacyDepartureState.h).
# Normal suite: always built and in ctest. The comprehensive differential acceptance job is built
# only with -DON_POINT_DIFFERENTIAL=ON (ctest label onpoint_differential, about 15-30 min serial;
# run it with `ctest -L onpoint_differential -j <n>`).
find_package(Python3 COMPONENTS Interpreter REQUIRED)
option(ON_POINT_DIFFERENTIAL "Build the comprehensive ON POINT departure differential job" OFF)

set(ON_POINT_DEPARTURE_FIXTURE_CPP ${CMAKE_CURRENT_BINARY_DIR}/OnPointCapacityFixtureData.cpp)
add_custom_command(
  OUTPUT ${ON_POINT_DEPARTURE_FIXTURE_CPP}
  COMMAND ${Python3_EXECUTABLE} ${REPO_ROOT}/test/fixtures/on_point/gen_capacity_fixture.py
    --out ${ON_POINT_DEPARTURE_FIXTURE_CPP}
  DEPENDS ${REPO_ROOT}/test/fixtures/on_point/gen_capacity_fixture.py
  COMMENT "Generating the ON POINT capacity fixture (departure tests)"
)

set(ON_POINT_SCHEDULE_CPP ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp)
set(ON_POINT_SHIPPED_DATA_CPP ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp)

function(on_point_departure_target name)
  target_include_directories(${name} PRIVATE ${REPO_ROOT}/src ${REPO_ROOT}/test/on_point_capacity/stubs)
  target_link_libraries(${name} PRIVATE crosspoint_test_common GTest::gtest_main)
endfunction()

add_executable(OnPointDepartureDiffTest OnPointDepartureDiffTest.cpp ${ON_POINT_SCHEDULE_CPP} ${ON_POINT_SHIPPED_DATA_CPP})
on_point_departure_target(OnPointDepartureDiffTest)
gtest_discover_tests(OnPointDepartureDiffTest)

add_executable(OnPointDepartureFixtureDiffTest OnPointDepartureFixtureDiffTest.cpp ${ON_POINT_SCHEDULE_CPP}
  ${ON_POINT_DEPARTURE_FIXTURE_CPP})
on_point_departure_target(OnPointDepartureFixtureDiffTest)
target_compile_definitions(OnPointDepartureFixtureDiffTest PRIVATE ON_POINT_CAPACITY_FIXTURE=1)
gtest_discover_tests(OnPointDepartureFixtureDiffTest)

# Benchmarks (not ctest).
foreach(variant shipped fixture)
  if(variant STREQUAL "shipped")
    set(data ${ON_POINT_SHIPPED_DATA_CPP})
    set(defs "")
  else()
    set(data ${ON_POINT_DEPARTURE_FIXTURE_CPP})
    set(defs ON_POINT_CAPACITY_FIXTURE=1)
  endif()
  add_executable(OnPointDepartureBench_${variant} OnPointDepartureBench.cpp ${ON_POINT_SCHEDULE_CPP} ${data})
  target_include_directories(OnPointDepartureBench_${variant} PRIVATE ${REPO_ROOT}/src
    ${REPO_ROOT}/test/on_point_capacity/stubs)
  target_link_libraries(OnPointDepartureBench_${variant} PRIVATE crosspoint_test_common GTest::gtest)
  target_compile_definitions(OnPointDepartureBench_${variant} PRIVATE BENCH_LABEL="${variant}" ${defs})
endforeach()

if(ON_POINT_DIFFERENTIAL)
  add_executable(OnPointDepartureShippedFullTest OnPointDepartureShippedFullTest.cpp ${ON_POINT_SCHEDULE_CPP}
    ${ON_POINT_SHIPPED_DATA_CPP})
  on_point_departure_target(OnPointDepartureShippedFullTest)
  gtest_discover_tests(OnPointDepartureShippedFullTest PROPERTIES LABELS onpoint_differential TIMEOUT 3600)

  add_executable(OnPointDepartureFixtureFullTest OnPointDepartureFixtureDiffTest.cpp ${ON_POINT_SCHEDULE_CPP}
    ${ON_POINT_DEPARTURE_FIXTURE_CPP})
  on_point_departure_target(OnPointDepartureFixtureFullTest)
  target_compile_definitions(OnPointDepartureFixtureFullTest PRIVATE ON_POINT_CAPACITY_FIXTURE=1
    ON_POINT_DIFFERENTIAL_FULL=1)
  gtest_discover_tests(OnPointDepartureFixtureFullTest PROPERTIES LABELS onpoint_differential TIMEOUT 3600)
endif()
