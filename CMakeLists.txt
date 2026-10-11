cmake_minimum_required(VERSION 3.10)
project(FileStorageBPT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

# Add executable
add_executable(code main.cpp)

# Link libraries if needed
# target_link_libraries(code pthread)