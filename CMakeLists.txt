cmake_minimum_required(VERSION 3.30)
set(CMAKE_COLOR_DIAGNOSTICS OFF)
project(unlambda_cc
        VERSION 0.1
        LANGUAGES C CXX)

set(CMAKE CXX_STANDARD 14)

add_subdirectory(src)
add_subdirectory(test)

