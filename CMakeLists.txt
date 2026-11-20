cmake_minimum_required(VERSION 3.16)
project(HW5_Simulador_RISCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(riscv_sim src/riscv_sim.cpp)

if(MSVC)
    target_compile_options(riscv_sim PRIVATE /W4)
else()
    target_compile_options(riscv_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()
