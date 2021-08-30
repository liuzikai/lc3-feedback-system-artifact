add_executable(klc3 klc3.cpp)
target_link_libraries(klc3 klc3Lib)
add_dependencies(klc3 lc3os_asm)
install(TARGETS klc3 RUNTIME DESTINATION bin)
target_compile_definitions(klc3 PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

configure_file(klc3-clean-output "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/klc3-clean-output" COPYONLY)
