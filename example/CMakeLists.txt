cmake_minimum_required(VERSION 3.31)
project(wolf_examples)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS json url)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Example: Router example
add_executable(example_router example_router.cpp)
target_link_libraries(example_router 
    PRIVATE 
    ${Boost_LIBRARIES}
)

# Example: Web server example
add_executable(example_web example_web.cpp)
target_link_libraries(example_web 
    PRIVATE 
    ${Boost_LIBRARIES}
)

# Example: HTTP Test Client
add_executable(test_client test_client.cpp)
target_link_libraries(test_client 
    PRIVATE 
    ${Boost_LIBRARIES}
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(example_router PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(example_web PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation (optional)
install(TARGETS example_router example_web test_client
    RUNTIME DESTINATION bin
)
