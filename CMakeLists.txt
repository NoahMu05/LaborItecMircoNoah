cmake_minimum_required(VERSION 3.10)
project(MyCProject C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

add_executable(MyCProject-ex
    src/main.c
    src/serial.c
    src/db.c
    src/util.c
    src/interp.c
    src/filter.c
    src/game.c
)

target_include_directories(MyCProject-ex PRIVATE src)
target_link_libraries(MyCProject-ex PRIVATE sqlite3)