cmake_minimum_required(VERSION 3.0)

include_directories(  ${CMAKE_SYSROOT}/opt/vc/include)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(SDLTossup)

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(wiringPi.h HAS_WIRING_PI)

# needed for raspberry pi, expect this is benign elsewhere.
link_directories("${CMAKE_SYSROOT}/opt/vc/lib")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})
add_subdirectory(external/SDL2-2.0.12)
add_executable(SDLTossup src/main.cpp  src/Path.cpp src/Point.h src/Path.h src/Outlines.cpp src/Outlines.h src/GameState.cpp src/LcdElement.cpp src/RpiGpio.cpp)
target_link_libraries(SDLTossup SDL2-static SDL2main)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # silence GCC or raspberry pi about using std::sort and the abi having changed
    target_compile_options(SDLTossup PRIVATE -Wno-psabi)
endif()

if (HAS_WIRING_PI)
    add_definitions(-DHAS_WIRING_PI)
    target_link_libraries(SDLTossup wiringPi)
endif()



add_custom_command(
        TARGET SDLTossup POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/resources
                ${CMAKE_CURRENT_BINARY_DIR})