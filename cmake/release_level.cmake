foreach(v CC FLAGS OBJS)
  string(REPLACE "|" ";" ${v} "${${v}}")
endforeach()

function(run)
  execute_process(COMMAND ${ARGN} COMMAND_ERROR_IS_FATAL ANY)
endfunction()

function(undefined_syms out)
  execute_process(
    COMMAND ${NM} -u --format=just-symbols ${OUT}
    OUTPUT_VARIABLE _u OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
  )
  string(REPLACE "\n" ";" _u "${_u}")
  set(${out} "${_u}" PARENT_SCOPE)
endfunction()

if(FORMAT STREQUAL "ELF")
  if(CC_ID STREQUAL "GNU")
    set(_rlink -flinker-output=nolto-rel)
  else()
    set(_rlink -fuse-ld=lld)
  endif()
  run(${CC} ${FLAGS} -O3 -nostdlib -r ${_rlink} ${OBJS} ${LIB} -o ${OUT})
  run(${OBJCOPY} --keep-global-symbol=xft_main ${OUT})
  run(${OBJCOPY} --redefine-sym xft_main=xft_main_${LEVEL} ${OUT})
  undefined_syms(_u)
  if(_u)
    message(FATAL_ERROR "release ${LEVEL}: ${OUT} has undefined symbols: ${_u}")
  endif()

elseif(FORMAT STREQUAL "COFF")
  execute_process(
    COMMAND
      ${LLD_LINK} /lldemit:llvm /entry:xft_main /include:xft_main /nodefaultlib
      /force:unresolved /machine:x64 /subsystem:console ${OBJS} ${LIB} /out:${OUT}.bc
    RESULT_VARIABLE _rc OUTPUT_VARIABLE _log ERROR_VARIABLE _log
  )
  if(NOT _rc EQUAL 0 OR NOT EXISTS ${OUT}.bc)
    message(FATAL_ERROR "release ${LEVEL}: lld-link LTO failed:\n${_log}")
  endif()
  run(${CC} -O3 -march=${MARCH} -ffunction-sections -fdata-sections -c ${OUT}.bc -o ${OUT})
  execute_process(
    COMMAND ${NM} --defined-only --extern-only --format=posix ${OUT}
    OUTPUT_VARIABLE _defs OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
  )
  string(REPLACE "\n" ";" _defs "${_defs}")
  set(_map "")
  foreach(d IN LISTS _defs)
    if(d MATCHES "^([^ ]+) ([^aA]) ")
      string(APPEND _map "${CMAKE_MATCH_1} ${CMAKE_MATCH_1}_${LEVEL}\n")
    endif()
  endforeach()
  file(WRITE ${OUT}.syms "${_map}")
  run(${OBJCOPY} --redefine-syms=${OUT}.syms ${OUT})
  undefined_syms(_u)
  execute_process(
    COMMAND ${NM} --defined-only --format=just-symbols ${OBJS} ${LIB}
    OUTPUT_VARIABLE _own OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
  )
  string(REPLACE "\n" ";" _own "${_own}")
  set(_lost "")
  foreach(u IN LISTS _u)
    if(u IN_LIST _own)
      list(APPEND _lost ${u})
    endif()
  endforeach()
  set(_u "${_lost}")
  if(_u)
    message(FATAL_ERROR "release ${LEVEL}: ${OUT} has undefined symbols: ${_u}")
  endif()

else()
  message(FATAL_ERROR "release_level.cmake: unknown FORMAT '${FORMAT}'")
endif()
