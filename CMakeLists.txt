cmake_minimum_required(VERSION 3.10)
project(Universe3D)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL (built into macOS)
find_package(OpenGL REQUIRED)

# Find GLFW (installed via Homebrew)
# Homebrew usually puts things in /opt/homebrew on Apple Silicon, or /usr/local on Intel
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Create the executable
add_executable(universe universe.cpp)

# Link the libraries
target_include_directories(universe PRIVATE ${GLFW_INCLUDE_DIRS})
target_link_libraries(universe PRIVATE ${GLFW_LIBRARIES} OpenGL::GL)

# On macOS, we need to explicitly link the frameworks
if(APPLE)
    target_link_libraries(universe PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
