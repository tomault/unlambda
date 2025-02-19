add_executable(array_tests array_tests.cpp testing_utils.cpp)

target_include_directories(array_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(array_tests PRIVATE ...)

target_link_directories(array_tests PUBLIC "/usr/local/lib")

target_link_libraries(array_tests libunlambda)
target_link_libraries(array_tests gtest_main gtest)
target_link_libraries(array_tests pthread)

add_executable(brkpt_tests brkpt_tests.cpp testing_utils.cpp)

target_include_directories(brkpt_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(brkpt_tests PRIVATE ...)

target_link_directories(brkpt_tests PUBLIC "/usr/local/lib")

target_link_libraries(brkpt_tests libunlambda)
target_link_libraries(brkpt_tests gtest_main gtest)
target_link_libraries(brkpt_tests pthread)

add_executable(fileio_tests fileio_tests.cpp)

target_include_directories(fileio_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(fileio_tests PRIVATE ...)

target_link_directories(fileio_tests PUBLIC "/usr/local/lib")

target_link_libraries(fileio_tests libunlambda)
target_link_libraries(fileio_tests gtest_main gtest)
target_link_libraries(fileio_tests pthread)

add_executable(stack_tests stack_tests.cpp)

target_include_directories(stack_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(stack_tests PRIVATE ...)

target_link_directories(stack_tests PUBLIC "/usr/local/lib")

target_link_libraries(stack_tests libunlambda)
target_link_libraries(stack_tests gtest_main gtest)
target_link_libraries(stack_tests pthread)

add_executable(symtab_tests symtab_tests.cpp)

target_include_directories(symtab_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(symtab_tests PRIVATE ...)

target_link_directories(symtab_tests PUBLIC "/usr/local/lib")

target_link_libraries(symtab_tests libunlambda)
target_link_libraries(symtab_tests gtest_main gtest)
target_link_libraries(symtab_tests pthread)

add_executable(vmmem_tests vmmem_tests.cpp testing_utils.cpp)

target_include_directories(vmmem_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vmmem_tests PRIVATE ...)

target_link_directories(vmmem_tests PUBLIC "/usr/local/lib")

target_link_libraries(vmmem_tests libunlambda)
target_link_libraries(vmmem_tests gtest_main gtest)
target_link_libraries(vmmem_tests pthread)

add_executable(vm_tests vm_tests.cpp testing_utils.cpp)

target_include_directories(vm_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vm_tests PRIVATE ...)

target_link_directories(vm_tests PUBLIC "/usr/local/lib")

target_link_libraries(vm_tests libunlambda)
target_link_libraries(vm_tests gtest_main gtest)
target_link_libraries(vm_tests pthread)

add_executable(dbgcmd_tests dbgcmd_tests.cpp testing_utils.cpp)

target_include_directories(dbgcmd_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(dbgcmd_tests PRIVATE ...)

target_link_directories(dbgcmd_tests PUBLIC "/usr/local/lib")

target_link_libraries(dbgcmd_tests libunlambda)
target_link_libraries(dbgcmd_tests gtest_main gtest)
target_link_libraries(dbgcmd_tests pthread)

add_executable(debug_tests debug_tests.cpp testing_utils.cpp)

target_include_directories(debug_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(debug_tests PRIVATE ...)

target_link_directories(debug_tests PUBLIC "/usr/local/lib")

target_link_libraries(debug_tests libunlambda)
target_link_libraries(debug_tests gtest_main gtest)
target_link_libraries(debug_tests pthread)

add_executable(vm_instructions_tests vm_instructions_tests.cpp)

target_include_directories(vm_instructions_tests PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vm_instructions_tests PRIVATE ...)

target_link_directories(vm_instructions_tests PUBLIC "/usr/local/lib")

target_link_libraries(vm_instructions_tests libunlambda)
target_link_libraries(vm_instructions_tests gtest_main gtest)
target_link_libraries(vm_instructions_tests pthread)

add_executable(vm_image_tests vm_image_tests.cpp testing_utils.cpp)

target_include_directories(vm_image_tests PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vm_image_tests PRIVATE ...)

target_link_directories(vm_image_tests PUBLIC "/usr/local/lib")

target_link_libraries(vm_image_tests libunlambda)
target_link_libraries(vm_image_tests gtest_main gtest)
target_link_libraries(vm_image_tests pthread)

add_executable(logging_tests logging_tests.cpp)

target_include_directories(logging_tests PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(logging_tests PRIVATE ...)

target_link_directories(logging_tests PUBLIC "/usr/local/lib")

target_link_libraries(logging_tests libunlambda)
target_link_libraries(logging_tests gtest_main gtest)
target_link_libraries(logging_tests pthread)

add_executable(argparse_tests argparse_tests.cpp)

target_include_directories(argparse_tests PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(argparse_tests PRIVATE ...)

target_link_directories(argparse_tests PUBLIC "/usr/local/lib")

target_link_libraries(argparse_tests libunlambda)
target_link_libraries(argparse_tests gtest_main gtest)
target_link_libraries(argparse_tests pthread)

add_executable(asm_tests asm_tests.cpp)

target_include_directories(asm_tests PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(asm_tests PRIVATE ...)

target_link_directories(asm_tests PUBLIC "/usr/local/lib")

target_link_libraries(asm_tests libunlambda)
target_link_libraries(asm_tests gtest_main gtest)
target_link_libraries(asm_tests pthread)

