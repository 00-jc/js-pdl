cmake_minimum_required(VERSION 3.21)
project(pdl VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "" FORCE)

add_subdirectory(xft)

add_executable(js-pdl)

target_sources(js-pdl PRIVATE
    src/entry_point.c
    src/lexer.c
    src/symbols.c
    src/errors.c
)

target_include_directories(js-pdl PRIVATE include)

target_link_libraries(js-pdl PRIVATE xft::xft xft::flags)

target_link_options(js-pdl PRIVATE $<$<CONFIG:Release>:LINKER:--gc-sections>)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_ok)
if(lto_ok)
    set_property(TARGET js-pdl PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
