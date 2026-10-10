# DOGEAR's TLS policy and issue parsing, run against the same wolfSSL 5.7.2 and
# ArduinoJson the firmware uses. Those come from PlatformIO's library cache, so
# build the X4 env once first (pio run -e default); without it this test is
# skipped rather than failing.
set(DOGEAR_LIBDEPS "${REPO_ROOT}/.pio/libdeps/default" CACHE PATH "PlatformIO libdeps for the DOGEAR host tests")
set(DOGEAR_WOLFSSL "${DOGEAR_LIBDEPS}/Arduino-wolfSSL/src")
set(DOGEAR_ARDUINOJSON "${DOGEAR_LIBDEPS}/ArduinoJson/src")
if(NOT EXISTS "${DOGEAR_WOLFSSL}/wolfssl/ssl.h" OR NOT EXISTS "${DOGEAR_ARDUINOJSON}/ArduinoJson.h")
  message(STATUS "DogearTlsTest skipped: run 'pio run -e default' once to fetch wolfSSL and ArduinoJson")
  return()
endif()

# Test certificates are generated here, in the build tree, never committed.
set(DOGEAR_CERT_DIR "${CMAKE_CURRENT_BINARY_DIR}/certs")
execute_process(COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/make_certs.sh" "${DOGEAR_CERT_DIR}"
  RESULT_VARIABLE DOGEAR_CERTS_RESULT OUTPUT_QUIET ERROR_VARIABLE DOGEAR_CERTS_ERROR)
if(NOT DOGEAR_CERTS_RESULT EQUAL 0)
  message(STATUS "DogearTlsTest skipped: could not generate test certificates (needs OpenSSL 3.4+): ${DOGEAR_CERTS_ERROR}")
  return()
endif()

file(GLOB DOGEAR_WOLFSSL_SOURCES "${DOGEAR_WOLFSSL}/src/*.c" "${DOGEAR_WOLFSSL}/wolfcrypt/src/*.c")
add_library(dogear_host_wolfssl STATIC ${DOGEAR_WOLFSSL_SOURCES})
target_include_directories(dogear_host_wolfssl PUBLIC ${DOGEAR_WOLFSSL})
# The firmware's wolfSSL flags: platformio.ini [base] build_flags (minus the
# client-only hint, so the test can also run a server) plus [dogear]
# build_flags (SHA-384 and SP P-384, which public CA chains need). Keep in sync.
target_compile_definitions(dogear_host_wolfssl PUBLIC
  WOLFSSL_USER_SETTINGS WOLFSSL_TLS13 WOLFSSL_HAVE_SP_ECC WOLFSSL_SP_SMALL
  HAVE_FFDHE_2048 HAVE_CURVE25519 HAVE_SNI HAVE_MAX_FRAGMENT
  WOLFSSL_SHA384 WOLFSSL_SP_384)
target_compile_options(dogear_host_wolfssl PRIVATE -w)

add_executable(DogearTlsTest
  DogearTlsTest.cpp
  ${REPO_ROOT}/src/activities/dogear/DogearTls.cpp
  ${REPO_ROOT}/src/activities/dogear/DogearIssue.cpp
)
target_include_directories(DogearTlsTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stub
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/activities/dogear
  ${REPO_ROOT}/test/dogear
  ${DOGEAR_ARDUINOJSON}
)
target_compile_definitions(DogearTlsTest PRIVATE
  DOGEAR_TLS_HOST_TEST DOGEAR_ENABLED DOGEAR_TEST_CERTS="${DOGEAR_CERT_DIR}"
  DOGEAR_REGULAR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/regular-issue.json")
target_link_libraries(DogearTlsTest PRIVATE crosspoint_test_common dogear_host_wolfssl GTest::gtest_main)
gtest_discover_tests(DogearTlsTest)
