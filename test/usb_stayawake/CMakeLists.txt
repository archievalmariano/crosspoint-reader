add_executable(UsbStayAwakeTest
  UsbStayAwakeTest.cpp
)
target_include_directories(UsbStayAwakeTest PRIVATE ${REPO_ROOT}/src)
target_link_libraries(UsbStayAwakeTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(UsbStayAwakeTest)
