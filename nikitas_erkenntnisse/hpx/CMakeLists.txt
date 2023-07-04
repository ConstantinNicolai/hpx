cmake_minimum_required(VERSION 3.0.0)
project(HPXTEST CXX)

#set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_COMPILER g++)
#set(CMAKE_BUILD_TYPE Debug)
#set(BUILD_SHARED_LIBS true)

find_package(benchmark REQUIRED)
find_package(HPX REQUIRED)


add_executable(my_hpx_program main.cpp)
#target_compile_options(my_hpx_program PRIVATE -DHPX_WITH_MAX_CPU_COUNT=128)
target_link_libraries(my_hpx_program HPX::hpx HPX::wrap_main HPX::iostreams_component)
target_link_libraries(my_hpx_program benchmark::benchmark)
