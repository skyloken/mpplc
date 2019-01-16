cmake_minimum_required(VERSION 3.13)
project(software5_ex4 C)

set(CMAKE_C_STANDARD 90)

add_executable(software5_ex4 src/mpplc.h src/mpplc.c src/scan.c src/parse.c src/id-list.c)