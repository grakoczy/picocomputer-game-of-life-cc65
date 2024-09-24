cmake_minimum_required(VERSION 3.13)

project(MY-RP6502-PROJECT C ASM)

add_subdirectory(tools)

add_executable(life)
rp6502_executable(life)
target_sources(life PRIVATE
    src/colors.c
    src/bitmap_graphics.c
    src/main.c
)
