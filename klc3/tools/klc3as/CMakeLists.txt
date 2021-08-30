add_executable(klc3as klc3as.cpp)
target_link_libraries(klc3as klc3Lib)
install(TARGETS klc3as RUNTIME DESTINATION bin)
target_compile_definitions(klc3as PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")