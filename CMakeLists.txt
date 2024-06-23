cmake_minimum_required(VERSION 3.10)

# Project definitions
set(PROJECT_NAME TileableWorleyGen)
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Project sources
include_directories(${PROJECT_SOURCE_DIR}/inc)
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# External libraries
find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)


add_executable(${PROJECT_NAME} ${SOURCES})

# Link directories and libraries if needed (example shown)
# target_link_libraries(${PROJECT_NAME} some_library)
target_link_libraries(${PROJECT_NAME} sfml-system sfml-window sfml-graphics sfml-network sfml-audio)