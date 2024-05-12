cmake_minimum_required(VERSION 3.7.2)

project(benchmark C)

add_executable(benchmark src/benchmark.c)
# target_include_directories( PRIVATE include)
target_link_libraries(benchmark sel4runtime muslc sel4 sel4muslcsys)

# warn about everything
add_compile_options(-Wall -Werror -W -Wextra)