cmake_minimum_required(VERSION 3.14)

project(JackCompiler VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Build
set(LIB_NAME jack-compiler-impl)
set(EXECUTABLE_NAME jack-compiler)

add_library(${LIB_NAME} OBJECT)

target_sources(${LIB_NAME} PRIVATE
                           src/CompilationEngine.cpp
                           src/JackCompiler.cpp
                           src/SymbolTable.cpp 
                           src/Tokenizer.cpp
                           src/VMWriter.cpp
                           include/CompilationEngine.h
                           include/JackCompiler.h 
                           include/SymbolTable.h 
                           include/Tokenizer.h
                           include/VMWriter.h
)

target_include_directories(${LIB_NAME} PUBLIC include)

add_executable(${EXECUTABLE_NAME})
target_sources(${EXECUTABLE_NAME} PRIVATE src/main.cpp)

target_link_libraries(${EXECUTABLE_NAME} ${LIB_NAME})

# If gcc is used, it is necessary to link stdc++fs for std::filename support
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(${EXECUTABLE_NAME} stdc++fs)
endif()

set_target_properties(${LIB_NAME} ${EXECUTABLE_NAME} PROPERTIES 
    CXX_STANDARD 17 
    CXX_STANDARD_REQUIRED YES 
    CXX_EXTENSIONS NO
)

# Testing
include(CTest)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
