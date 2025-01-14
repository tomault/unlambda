
add_library(libunlambda STATIC argparse.c array.c brkpt.c dbgcmd.c debug.c
                               fileio.c logging.c stack.c symtab.c unlcc.c vm.c
			       vm_image.c vm_instructions.c vmmem.c)

target_include_directories(libunlambda PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(unl unl.c)

target_include_directories(unl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(unl libunlambda)
target_link_libraries(unl pthread)
