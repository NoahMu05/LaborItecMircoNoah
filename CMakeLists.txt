cmake_minimum_required(VERSION 3.10)
project(MyCProject)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_COMPILER "/usr/bin/gcc")

find_package(SQLite3 REQUIRED)

add_executable(LaborAufgaben-ex src/LaborAufgaben.c src/itec.c src/filter.c src/automode.c)
target_link_libraries(LaborAufgaben-ex SQLite::SQLite3 m)


