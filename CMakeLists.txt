cmake_minimum_required(VERSION 3.14)
project(C__)

set(CMAKE_CXX_STANDARD 17)

find_package(Armadillo REQUIRED)
include_directories(${ARMADILLO_INCLUDE_DIRS})

add_executable(C__ main.cpp stockcovrank.cpp stockcovrank.h)

target_link_libraries(C__ ${ARMADILLO_LIBRARIES})