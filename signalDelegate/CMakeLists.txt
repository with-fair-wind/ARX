cmake_minimum_required(VERSION 3.10)
project(signalDelegate LANGUAGES CXX)

option(EVT_USE_CPP14 "Use C++14 event bus implementation" ON)

if(EVT_USE_CPP14)
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/demo.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(EVT_USE_CPP14)
    target_compile_definitions(${PROJECT_NAME} PRIVATE EVT_USE_CPP14)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
