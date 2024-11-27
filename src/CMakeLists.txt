
add_library(libunlambda STATIC fileio.c stack.c symtab.c unlcc.c vm.c
                               vm_image.c vm_instructions.c vmmem.c)

target_include_directories(libunlambda PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
