cmake_minimum_required(VERSION 3.16)
project(CampusLibraryManager VERSION 1.0.0 LANGUAGES CXX)

add_executable(campus-library-manager
    src/main.cpp
    src/Library.cpp
)

target_include_directories(campus-library-manager PRIVATE include)
target_compile_features(campus-library-manager PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(campus-library-manager PRIVATE /W4 /permissive-)
else()
    target_compile_options(campus-library-manager PRIVATE -Wall -Wextra -Wpedantic)
endif()
