cmake_minimum_required(VERSION 3.0.0)
project(wordle-solver VERSION 0.0.0)

add_executable(wordle-solver main.c solver.c)

target_link_libraries(wordle-solver m)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

target_compile_options(wordle-solver PRIVATE -Wpedantic -Werror -Wall -Wextra)
