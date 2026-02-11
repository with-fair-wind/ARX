cmake_minimum_required(VERSION 3.10)
project(delegate_event_cpp14 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(delegate_event_demo
    src/main.cpp
)

target_include_directories(delegate_event_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(delegate_event_demo PRIVATE /utf-8)
endif()
