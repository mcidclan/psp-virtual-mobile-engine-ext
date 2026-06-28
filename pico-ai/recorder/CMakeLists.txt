cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE "$ENV{PSPDEV}/psp/share/pspdev.cmake" CACHE PATH "toolchain file")
project(pico-rec C CXX ASM)

add_executable(pico-rec
  main.cpp
)

target_include_directories(pico-rec PRIVATE ../)

target_compile_options(pico-rec PRIVATE -Os -G0 -Wall -fno-pic)

target_compile_options(pico-rec PRIVATE 
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)

target_link_libraries(pico-rec
  me-core psppower pspge pspdisplay pspctrl pspdebug
)

create_pbp_file(TARGET pico-rec TITLE "Pico AI PoC: Recorder" BUILD_PRX)
