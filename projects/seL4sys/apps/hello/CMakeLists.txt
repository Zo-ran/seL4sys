cmake_minimum_required(VERSION 3.7.2)

project(hello C)

add_executable(hello src/hello.c)
# target_include_directories( PRIVATE include)
target_link_libraries(hello sel4runtime muslc sel4 sel4muslcsys)

# warn about everything
add_compile_options(-Wall -Werror -W -Wextra)