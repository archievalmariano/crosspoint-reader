# OnPointStore (Stage 2, P2) against the firmware's JSON library: the same pinned ArduinoJson 7.4.2
# tarball test/dogear_tls downloads (SHA-256 checked there), or a local PlatformIO copy of it.
set(ON_POINT_ARDUINOJSON "")
foreach(candidate "${CMAKE_BINARY_DIR}/dogear_tls/deps/arduinojson/src")
  if(EXISTS "${candidate}/ArduinoJson.h")
    set(ON_POINT_ARDUINOJSON "${candidate}")
  endif()
endforeach()
if(NOT ON_POINT_ARDUINOJSON)
  file(GLOB ON_POINT_PIO_ARDUINOJSON "${REPO_ROOT}/.pio/libdeps/*/ArduinoJson/src/ArduinoJson/version.hpp")
  foreach(version_file IN LISTS ON_POINT_PIO_ARDUINOJSON)
    file(STRINGS "${version_file}" version_line REGEX "#define ARDUINOJSON_VERSION \"7\\.4\\.2\"")
    if(version_line AND NOT ON_POINT_ARDUINOJSON)
      get_filename_component(dir "${version_file}" DIRECTORY)
      get_filename_component(ON_POINT_ARDUINOJSON "${dir}" DIRECTORY)
    endif()
  endforeach()
endif()
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
