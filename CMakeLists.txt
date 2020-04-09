cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(SDLTossup)

# needed for raspberry pi, expect this is benign elsewhere.
link_directories(/opt/vc/lib)




set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})
add_subdirectory(external/SDL2-2.0.12)
add_executable(SDLTossup src/main.cpp  src/Path.cpp src/Point.h src/Path.h src/Outlines.cpp src/Outlines.h)
target_link_libraries(SDLTossup SDL2 SDL2main)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # silence GCC or raspberry pi about using std::sort and the abi having changed
    target_compile_options(SDLTossup PRIVATE -Wno-psabi)
endif()
