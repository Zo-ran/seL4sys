cmake_minimum_required(VERSION 3.7.2)

project(endless C)

add_executable(endless src/endless.c)
# target_include_directories( PRIVATE include)
target_link_libraries(endless sel4runtime muslc sel4 sel4muslcsys)

# warn about everything
add_compile_options(-Wall -Werror -W -Wextra)