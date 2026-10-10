# DOGEAR's TLS policy and issue parsing, run against the same libraries the
# firmware uses (platformio.ini lib_deps): the PlatformIO registry tarballs of
# Arduino-wolfSSL 5.7.2 and ArduinoJson 7.4.2, pinned by SHA-256, with
# CrossPoint's wolfSSL overrides applied by scripts/patch_wolfssl.py exactly as
# a firmware build applies them. Test certificates are generated in the build
# tree by make_certs.py, with the hash-pinned `cryptography` in requirements.txt.
#
# Without a dependency (offline, or no such Python), the test is skipped, unless
# DOGEAR_REQUIRE_TLS_TESTS is ON (CI), where that is a configure error.
option(DOGEAR_REQUIRE_TLS_TESTS "Fail instead of skipping DogearTlsTest when its dependencies are missing" OFF)
set(DOGEAR_CERTS_PYTHON "" CACHE FILEPATH "A Python with test/dogear_tls/requirements.txt (default: python3)")

macro(dogear_tls_unavailable reason)
  if(DOGEAR_REQUIRE_TLS_TESTS)
    message(FATAL_ERROR "DogearTlsTest is required, but ${reason}")
  endif()
  message(STATUS "DogearTlsTest skipped: ${reason}")
  return()
endmacro()

set(DOGEAR_DEPS "${CMAKE_CURRENT_BINARY_DIR}/deps")
set(DOGEAR_DEP_NAMES wolfssl arduinojson)
set(DOGEAR_DEP_URL_wolfssl
    "https://dl.registry.platformio.org/download/wolfssl/library/Arduino-wolfSSL/5.7.2/Arduino-wolfSSL-5.7.2.tar.gz")
set(DOGEAR_DEP_SHA_wolfssl 860a2b63adb25a3acb778f20e094449418150558b6f809c6564176b8f3babf3e)
set(DOGEAR_DEP_URL_arduinojson
    "https://dl.registry.platformio.org/download/bblanchon/library/ArduinoJson/7.4.2/ArduinoJson-7.4.2.tar.gz")
set(DOGEAR_DEP_SHA_arduinojson 64fcea6e0e19a049054d47cae5b69baedbe1dcd44d46fab7f7415559127b7f34)
foreach(dep IN LISTS DOGEAR_DEP_NAMES)
  set(tarball "${DOGEAR_DEPS}/${dep}.tar.gz")
  if(EXISTS "${tarball}")
    file(SHA256 "${tarball}" have)
    if(NOT have STREQUAL DOGEAR_DEP_SHA_${dep})
      file(REMOVE "${tarball}")
    endif()
  endif()
  if(NOT EXISTS "${tarball}")
    file(DOWNLOAD "${DOGEAR_DEP_URL_${dep}}" "${tarball}.part" STATUS status TLS_VERIFY ON)
    list(GET status 0 code)
    if(NOT code EQUAL 0)
      file(REMOVE "${tarball}.part")
      dogear_tls_unavailable("${dep} could not be downloaded (${status})")
    endif()
    file(SHA256 "${tarball}.part" have)
    if(NOT have STREQUAL DOGEAR_DEP_SHA_${dep})
      file(REMOVE "${tarball}.part")
      message(FATAL_ERROR "${dep}: SHA-256 ${have} is not the pinned ${DOGEAR_DEP_SHA_${dep}}")
    endif()
    file(RENAME "${tarball}.part" "${tarball}")
  endif()
  # Extracted afresh on every configure, so the patch always meets pristine sources.
  file(REMOVE_RECURSE "${DOGEAR_DEPS}/${dep}")
  file(ARCHIVE_EXTRACT INPUT "${tarball}" DESTINATION "${DOGEAR_DEPS}/${dep}")
endforeach()
set(DOGEAR_WOLFSSL "${DOGEAR_DEPS}/wolfssl/src")
set(DOGEAR_ARDUINOJSON "${DOGEAR_DEPS}/arduinojson/src")
if(NOT EXISTS "${DOGEAR_WOLFSSL}/wolfssl/ssl.h" OR NOT EXISTS "${DOGEAR_ARDUINOJSON}/ArduinoJson.h")
  message(FATAL_ERROR "the pinned wolfSSL or ArduinoJson tarball has an unexpected layout")
endif()

if(DOGEAR_CERTS_PYTHON)
  set(DOGEAR_PYTHON "${DOGEAR_CERTS_PYTHON}")
else()
  find_program(DOGEAR_PYTHON python3)
endif()
if(NOT DOGEAR_PYTHON)
  dogear_tls_unavailable("no python3 for scripts/patch_wolfssl.py and make_certs.py")
endif()
execute_process(COMMAND "${DOGEAR_PYTHON}" "${REPO_ROOT}/scripts/patch_wolfssl.py" "${DOGEAR_WOLFSSL}/user_settings.h"
  RESULT_VARIABLE DOGEAR_PATCH_RESULT OUTPUT_QUIET ERROR_VARIABLE DOGEAR_PATCH_ERROR)
if(NOT DOGEAR_PATCH_RESULT EQUAL 0)
  message(FATAL_ERROR "scripts/patch_wolfssl.py failed: ${DOGEAR_PATCH_ERROR}")
endif()

# Test certificates are generated here, in the build tree, never committed.
set(DOGEAR_CERT_DIR "${CMAKE_CURRENT_BINARY_DIR}/certs")
execute_process(COMMAND "${DOGEAR_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/make_certs.py" "${DOGEAR_CERT_DIR}"
  RESULT_VARIABLE DOGEAR_CERTS_RESULT OUTPUT_QUIET ERROR_VARIABLE DOGEAR_CERTS_ERROR)
if(NOT DOGEAR_CERTS_RESULT EQUAL 0)
  dogear_tls_unavailable("make_certs.py failed (install test/dogear_tls/requirements.txt): ${DOGEAR_CERTS_ERROR}")
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
