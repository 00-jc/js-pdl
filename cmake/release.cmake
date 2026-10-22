# multi level release (JSPDL_RELEASE=LINUX_x86 or WIN64_x86).

set(JSPDL_LEVELS v1 v2 v3 v4)

set(JSPDL_MARCH_v1 x86-64)
foreach(n 2 3 4)
  set(JSPDL_MARCH_v${n} x86-64-v${n})
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(_os WIN64)
  set(_format COFF)
else()
  set(_os LINUX)
  set(_format ELF)
endif()

if(JSPDL_LEVEL)
  if(NOT JSPDL_LEVEL IN_LIST JSPDL_LEVELS)
    message(FATAL_ERROR "jspdl: unknown JSPDL_LEVEL '${JSPDL_LEVEL}'")
  endif()

  add_subdirectory(xft)
  set_property(TARGET xft PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)

  add_library(js-pdl-objs OBJECT ${JSPDL_SRCS})
  target_include_directories(js-pdl-objs PRIVATE include)
  target_link_libraries(js-pdl-objs PRIVATE xft::xft xft::flags)
  set_property(TARGET js-pdl-objs PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)

  set(_o ${CMAKE_CURRENT_BINARY_DIR}/js-pdl_${JSPDL_LEVEL}${CMAKE_C_OUTPUT_EXTENSION})
  set(_cc ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1})
  if(CMAKE_C_COMPILER_TARGET)
    list(APPEND _cc --target=${CMAKE_C_COMPILER_TARGET})
  endif()
  find_program(JSPDL_LLVM_LINK llvm-link)
  find_program(JSPDL_OPT opt)
  find_program(JSPDL_LLC llc)
  if(_format STREQUAL "COFF" AND NOT (JSPDL_LLVM_LINK AND JSPDL_OPT AND JSPDL_LLC))
    message(FATAL_ERROR "jspdl: WIN64_x86 needs llvm-link, opt and llc")
  endif()

  add_custom_command(
    OUTPUT ${_o}
    COMMAND
      ${CMAKE_COMMAND}
      -DLEVEL=${JSPDL_LEVEL}
      -DFORMAT=${_format}
      "-DOBJS=$<JOIN:$<TARGET_OBJECTS:js-pdl-objs>,|>"
      -DLIB=$<TARGET_FILE:xft>
      -DOUT=${_o}
      "-DCC=$<JOIN:${_cc},|>"
      -DCC_ID=${CMAKE_C_COMPILER_ID}
      "-DFLAGS=$<JOIN:$<TARGET_PROPERTY:xft_flags,INTERFACE_COMPILE_OPTIONS>;$<TARGET_PROPERTY:xft,INTERFACE_COMPILE_OPTIONS>,|>"
      -DNM=${CMAKE_NM}
      -DOBJCOPY=${CMAKE_OBJCOPY}
      -DLLVM_LINK=${JSPDL_LLVM_LINK}
      -DOPT=${JSPDL_OPT}
      -DLLC=${JSPDL_LLC}
      -DLLC_CPU=${JSPDL_MARCH_${JSPDL_LEVEL}}
      -P ${PROJECT_SOURCE_DIR}/cmake/release_level.cmake
    DEPENDS
      $<TARGET_OBJECTS:js-pdl-objs> xft ${PROJECT_SOURCE_DIR}/cmake/release_level.cmake
    COMMENT "jspdl: squashing js-pdl ${JSPDL_LEVEL} into ${_o}"
    VERBATIM
  )
  add_custom_target(js-pdl_${JSPDL_LEVEL} ALL DEPENDS ${_o})
  return()
endif()

if(NOT JSPDL_RELEASE STREQUAL "${_os}_x86"
    OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  message(
    FATAL_ERROR
    "jspdl: JSPDL_RELEASE=${JSPDL_RELEASE} but the target is "
    "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}; use the matching preset "
    "or toolchain file"
  )
endif()

# the trampoline runs before cpuid says anything, so its xft is the baseline
set(XFT_ARCH ${JSPDL_MARCH_v1} CACHE STRING "" FORCE)
add_subdirectory(xft)

include(ExternalProject)

set(_fwd -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND _fwd -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
  if(DEFINED JSPDL_WIN64_SYSROOT)
    list(APPEND _fwd -DJSPDL_WIN64_SYSROOT=${JSPDL_WIN64_SYSROOT})
  endif()
else()
  list(APPEND _fwd -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
endif()

set(_objs "")
foreach(l IN LISTS JSPDL_LEVELS)
  set(_dir ${CMAKE_CURRENT_BINARY_DIR}/${l})
  set(_o ${_dir}/js-pdl_${l}${CMAKE_C_OUTPUT_EXTENSION})
  ExternalProject_Add(
    js-pdl-${l}
    SOURCE_DIR ${PROJECT_SOURCE_DIR}
    BINARY_DIR ${_dir}
    CMAKE_ARGS ${_fwd} -DJSPDL_LEVEL=${l} -DXFT_ARCH=${JSPDL_MARCH_${l}}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target js-pdl_${l}
    BUILD_BYPRODUCTS ${_o}
    BUILD_ALWAYS ON
    INSTALL_COMMAND ""
  )
  list(APPEND _objs ${_o})
endforeach()
set_source_files_properties(${_objs} PROPERTIES EXTERNAL_OBJECT ON GENERATED ON)

add_executable(js-pdl src/trampoline.c ${_objs})
foreach(l IN LISTS JSPDL_LEVELS)
  add_dependencies(js-pdl js-pdl-${l})
endforeach()
target_link_libraries(js-pdl PRIVATE xft::xft xft::flags)
target_link_options(js-pdl PRIVATE LINKER:--gc-sections LINKER:--strip-all)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_ok)
if(lto_ok)
  set_property(TARGET js-pdl PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()
