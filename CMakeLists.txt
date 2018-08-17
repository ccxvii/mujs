project(mujs C)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "-std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Os")


ADD_DEFINITIONS(-DHAVE_READLINE)

SET(MUJS_SRC
jsarray.c
jsboolean.c
jsbuiltin.c
jscompile.c
jsdate.c
jsdtoa.c
jsdump.c
jserror.c
jsfunction.c
jsgc.c
jsintern.c
jslex.c
jsmath.c
jsnumber.c
jsobject.c
json.c
jsparse.c
jsproperty.c
jsregexp.c
jsrun.c
jsstate.c
jsstring.c
jsvalue.c
regexp.c
utf.c
utftype.c
)

add_library(mujs  ${MUJS_SRC})


add_executable(mujs-pp  pp.c)
target_link_libraries(mujs-pp mujs m)
INSTALL(TARGETS mujs-pp DESTINATION bin)

add_executable(mujs_bin  main.c)
set_target_properties(mujs_bin PROPERTIES  OUTPUT_NAME  mujs)
target_link_libraries(mujs_bin mujs m readline)
INSTALL(TARGETS mujs_bin DESTINATION bin)


INSTALL(FILES  "mujs.h"  DESTINATION   include   )
