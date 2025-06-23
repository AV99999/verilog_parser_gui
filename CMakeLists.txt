cmake_minimum_required(VERSION 3.10)
project(VerilogConsoleApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# Enable GUI or CLI via flag
option(BUILD_GUI "Build Qt GUI with Tcl shell" ON)

add_subdirectory(src)

