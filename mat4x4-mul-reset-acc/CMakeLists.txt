cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE "$ENV{PSPDEV}/psp/share/pspdev.cmake" CACHE PATH "toolchain file")
project(vme-mat C CXX ASM)

add_executable(vme-mat
  main.cpp
)

target_include_directories(vme-mat PRIVATE ../)

target_compile_options(vme-mat PRIVATE -Os -G0 -Wall -fno-pic)

target_compile_options(vme-mat PRIVATE 
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)

target_link_libraries(vme-mat
  me-core psppower pspge pspdisplay pspctrl pspdebug
)

create_pbp_file(TARGET vme-mat TITLE "Mat4x4 Mult by Vector, Reset Acc" BUILD_PRX)
