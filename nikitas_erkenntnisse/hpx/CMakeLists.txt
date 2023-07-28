cmake_minimum_required(VERSION 3.0.0)
project(HPXTEST CXX)

#set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_COMPILER g++)
#set(CMAKE_BUILD_TYPE Debug)
#set(BUILD_SHARED_LIBS true)

find_package(benchmark REQUIRED)
find_package(HPX REQUIRED)

add_executable(reduction reduction.cpp)
target_link_libraries(reduction HPX::hpx HPX::wrap_main HPX::iostreams_component)
target_link_libraries(reduction benchmark::benchmark)
