cmake_minimum_required(VERSION 3.12)

set(PROJECT_TESTS_NAME ${PROJECT_NAME}Tests)

# Files
set(TEST_SOURCES main.cpp
				 CompilationEngineTests.cpp
)

add_executable(${PROJECT_TESTS_NAME} ${TEST_SOURCES})
target_link_libraries(${PROJECT_TESTS_NAME} gtest ${PROJECT_LIB_NAME})

# If gcc is used, it is necessary to link stdc++fs for std::filename support
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(${PROJECT_TESTS_NAME} stdc++fs)
endif()

set_target_properties(${PROJECT_TESTS_NAME} PROPERTIES 
    CXX_STANDARD 17 
    CXX_STANDARD_REQUIRED YES 
    CXX_EXTENSIONS NO
)

# Add tests and pass test-files directory path as an argument to gtest
# Note: Testing directly from the test-explorer in VS Studio 2017 (tested with version 15.9.11) does not seem
#       to take the argument into account, use ctest instead.  
add_test(NAME ${PROJECT_TESTS_NAME} COMMAND ${PROJECT_TESTS_NAME} "${CMAKE_CURRENT_LIST_DIR}/test-files/")