# The production OnPointActivity on the host, against the stand-ins in stubs/ (searched first,
# so they shadow the device headers it includes), with OnPointStore over FakeStorage.
include(${REPO_ROOT}/test/on_point_store/ArduinoJson.cmake)
if(NOT ON_POINT_ARDUINOJSON)
  message(WARNING "OnPointActivity tests skipped: no ArduinoJson 7.4.2 (see test/on_point_store/ArduinoJson.cmake)")
  return()
endif()

set(ON_POINT_ACTIVITY_SOURCES
  OnPointActivityHarness.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointActivity.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRegions.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteList.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointRouteResolution.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointSchedule.cpp
  ${REPO_ROOT}/src/activities/on_point/OnPointStore.cpp
)
set(SHIPPED_DATA ${REPO_ROOT}/src/activities/on_point/OnPointScheduleData.cpp)
set(SHIPPED_REGIONS ${REPO_ROOT}/src/activities/on_point/OnPointRegionData.cpp)

add_executable(OnPointActivityTest OnPointActivityTest.cpp ${ON_POINT_ACTIVITY_SOURCES} ${SHIPPED_DATA}
  ${SHIPPED_REGIONS})
add_executable(OnPointActivityEmptyCatalogueTest OnPointActivityEmptyCatalogueTest.cpp ${ON_POINT_ACTIVITY_SOURCES}
  EmptyCatalogue.cpp ${SHIPPED_REGIONS})
add_executable(OnPointActivityInvalidCatalogueTest OnPointActivityInvalidCatalogueTest.cpp
  ${ON_POINT_ACTIVITY_SOURCES} InvalidCatalogue.cpp ${SHIPPED_REGIONS})
add_executable(OnPointActivityOneCollectionTest OnPointActivityConfigTest.cpp ${ON_POINT_ACTIVITY_SOURCES}
  ${SHIPPED_DATA} TestRegions.cpp)
target_compile_definitions(OnPointActivityOneCollectionTest PRIVATE ON_POINT_TEST_ONE_COLLECTION=1)
add_executable(OnPointActivityOneRouteCollectionTest OnPointActivityConfigTest.cpp ${ON_POINT_ACTIVITY_SOURCES}
  ${SHIPPED_DATA} TestRegions.cpp)
foreach(target OnPointActivityTest OnPointActivityEmptyCatalogueTest OnPointActivityInvalidCatalogueTest
    OnPointActivityOneCollectionTest OnPointActivityOneRouteCollectionTest)
  target_include_directories(${target} BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs)
  target_include_directories(${target} PRIVATE ${REPO_ROOT}/src ${REPO_ROOT}/lib/Memory ${ON_POINT_ARDUINOJSON})
  target_compile_definitions(${target} PRIVATE ON_POINT_ENABLED=1)
  target_link_libraries(${target} PRIVATE crosspoint_test_common GTest::gtest_main)
  gtest_discover_tests(${target})
endforeach()

# Collection labels against the real built-in fonts and English strings.
add_executable(OnPointLabelWidthTest OnPointLabelWidthTest.cpp ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp)
target_include_directories(OnPointLabelWidthTest PRIVATE ${REPO_ROOT}/lib/EpdFont ${REPO_ROOT}/lib/Utf8)
target_compile_definitions(OnPointLabelWidthTest PRIVATE
  ON_POINT_ENGLISH_YAML="${REPO_ROOT}/lib/I18n/translations/english.yaml")
target_link_libraries(OnPointLabelWidthTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(OnPointLabelWidthTest)
