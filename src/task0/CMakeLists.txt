cmake_minimum_required(VERSION 3.16)

project(task0 C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -O3 -march=native -mtune=native -flto")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g -flto")

include_directories(src/include)

set(COMMON_SOURCES
    src/parse_arg.c
)

set(V2_SOURCES
    src/vector_v2_io.c
    src/vector_v2_getters.c
    src/vector_v2_algo.c
    src/thread_pool.c
    ${COMMON_SOURCES}
)

add_executable(${PROJECT_NAME}
    src/main.c
    ${V2_SOURCES}
)