find_package(FLEX REQUIRED)

FLEX_TARGET(lc3_lex lc3.f ${CMAKE_CURRENT_BINARY_DIR}/lex.lc3.c COMPILE_FLAGS "-i -Plc3")
add_executable(lc3as ${FLEX_lc3_lex_OUTPUTS} symbol.c)
target_include_directories(lc3as PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(OUTPUT lc3os.asm
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lc3os.asm ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lc3os.asm
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/lc3os.asm)
add_custom_target(lc3os_asm DEPENDS lc3os.asm)

add_custom_command(OUTPUT lc3os.sym lc3os.obj
        COMMAND lc3as ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lc3os.asm > /dev/null
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/lc3os.asm
        DEPENDS lc3as)
add_custom_target(lc3os_compiled DEPENDS lc3os_asm lc3os.sym lc3os.obj)

add_executable(lc3sim
        lc3.def
        lc3sim.c
        lc3sim.h
        symbol.c
        symbol.h)
target_compile_definitions(lc3sim PRIVATE
        INSTALL_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        MAP_LOCATION_TO_SYMBOL)
find_package(Readline)
if (READLINE_FOUND)
    message("Build lc3sim with readline.")
    target_compile_definitions(lc3sim PRIVATE
            USE_READLINE)
    target_link_libraries(lc3sim readline)
else()
    message("Readline not found. Build lc3sim without readline.")
endif()

add_dependencies(lc3sim lc3os_compiled)

# NOTICE: since the path of lc3os is hard-coded in the source, this target is not suitable for install