cmake_minimum_required(VERSION 3.7.2)

project(ostest C)

add_executable(test src/test.c)
target_include_directories(test PRIVATE include)
target_link_libraries(test sel4runtime muslc sel4 libsyscall)

# warn about everything
add_compile_options(-Wall -Werror -W -Wextra)

add_app(test)