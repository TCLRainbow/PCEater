cmake_minimum_required(VERSION 3.19)
project(PCEater)

set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "-fopenmp")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -ffast-math")

include_directories(
        lib/boost
        lib/opencl/external/OpenCL-Headers)
file(WRITE ${PROJECT_SOURCE_DIR}/flags.h "#define PCEaterFlags \"${CMAKE_CXX_FLAGS}\"")

add_executable(PCEater PCEater.cpp)
target_link_libraries(PCEater ${PROJECT_SOURCE_DIR}/lib/libOpenCL.a)