cmake_minimum_required(VERSION 4.1.2)
project(wolf_tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS json url)
find_package(Catch2 3 REQUIRED)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})

# Create router test executable
add_executable(router_tests router_test.cpp)
target_link_libraries(router_tests 
    PRIVATE 
    Catch2::Catch2WithMain
    ${Boost_LIBRARIES}
)

# Create web server test executable
add_executable(web_server_tests web_server_test.cpp)
target_link_libraries(web_server_tests 
    PRIVATE 
    Catch2::Catch2WithMain
    ${Boost_LIBRARIES}
)

# Create http_response test executable
add_executable(http_response_tests http_response_test.cpp)
target_link_libraries(http_response_tests 
    PRIVATE 
    Catch2::Catch2WithMain
    ${Boost_LIBRARIES}
)

# Enable testing
enable_testing()
add_test(NAME router_tests COMMAND router_tests)
add_test(NAME web_server_tests COMMAND web_server_tests)
add_test(NAME http_response_tests COMMAND http_response_tests)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(router_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(web_server_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(http_response_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()