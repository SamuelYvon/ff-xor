cmake_minimum_required(VERSION 3.25)
project(ff-xor C)

set(CMAKE_C_STANDARD 23)

add_executable(ff-xor ff-xor.c)
target_sources(ff-xor PRIVATE ff-xor.c)

if (MSVC)
    target_compile_options(ff-xor PRIVATE /W4 /WX)
else ()
    target_compile_options(ff-xor PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif ()
