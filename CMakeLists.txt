cmake_minimum_required(VERSION 3.21)

set(JSPDL_RELEASE "" CACHE STRING "LINUX_x86, WIN64_x86, or empty for one build for XFT_ARCH")
set_property(CACHE JSPDL_RELEASE PROPERTY STRINGS "" LINUX_x86 WIN64_x86)
set(JSPDL_LEVEL "" CACHE STRING "internal: v1 .. v4 sub-build of a release")

project(pdl VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "" FORCE)

set(JSPDL_SRCS
    src/entry_point.c
	src/lexer.c
    src/errors.c
    src/symbols.c
)

if(JSPDL_RELEASE OR JSPDL_LEVEL)
    include(cmake/release.cmake)
    return()
endif()

add_subdirectory(xft)

add_executable(js-pdl)

target_sources(js-pdl PRIVATE ${JSPDL_SRCS})

target_include_directories(js-pdl PRIVATE include)

target_link_libraries(js-pdl PRIVATE xft::xft xft::flags)

target_link_options(js-pdl PRIVATE $<$<CONFIG:Release>:LINKER:--gc-sections>)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_ok)
if(lto_ok)
    set_property(TARGET js-pdl PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()
