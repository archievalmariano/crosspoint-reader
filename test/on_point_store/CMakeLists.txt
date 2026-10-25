# OnPointStore (Stage 2, P2) against the firmware's JSON library: the same pinned ArduinoJson 7.4.2
# tarball test/dogear_tls downloads (SHA-256 checked there), or a local PlatformIO copy of it.
include(${REPO_ROOT}/test/on_point_store/ArduinoJson.cmake)
if(NOT ON_POINT_ARDUINOJSON)
  message(WARNING "OnPointStoreTest skipped: no ArduinoJson 7.4.2 (the test/dogear_tls download failed and no .pio copy)")
  return()
endif()
message(STATUS "OnPointStoreTest: ArduinoJson from ${ON_POINT_ARDUINOJSON}")

add_executable(OnPointStoreTest OnPointStoreTest.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointStore.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp)
target_include_directories(OnPointStoreTest PRIVATE
  ${REPO_ROOT}/test/on_point_route_list/stubs  # no-op Logging.h
  ${REPO_ROOT}/lib/Memory
  ${REPO_ROOT}/src
  ${ON_POINT_ARDUINOJSON})
target_link_libraries(OnPointStoreTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointStoreTest)
