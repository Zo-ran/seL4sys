include(settings.cmake)
cmake_minimum_required(VERSION 3.7.2)

project(seL4sys C ASM)

find_package(seL4 REQUIRED)
find_package(elfloader-tool REQUIRED)
find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)
find_package(sel4_projects_libs REQUIRED)

sel4_import_kernel()
elfloader_import_project()

musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()
util_libs_import_libraries()
sel4_projects_libs_import_libraries()
sel4_libs_import_libraries()

add_subdirectory(apps/shell)
add_subdirectory(apps/benchmark)
add_subdirectory(apps/hello)
add_subdirectory(apps/endless)
add_subdirectory(os)

include(simulation)
GenerateSimulateScript()