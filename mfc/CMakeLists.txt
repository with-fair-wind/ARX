project(mfc)

file(GLOB_RECURSE INCS CONFIGURE_DEPENDS inc/*.h)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.cpp src/*.def)
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS resources/*.rc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS} ${INCS} ${RESOURCES})

add_library(${PROJECT_NAME} SHARED ${INCS} ${SRCS} ${RESOURCES})

# target_precompile_headers(${PROJECT_NAME} PUBLIC inc/stdafx.h)

if(MSVC)
    # MFC设置：使用共享MFC DLL
    target_compile_definitions(${PROJECT_NAME} PRIVATE _AFXDLL _AFXEXT _WIN32_WINNT=0x0600)
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        CMAKE_MFC_FLAG 2
        VS_GLOBAL_UseOfMfc "Dynamic"
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/$<CONFIG>/bin
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/$<CONFIG>/lib
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/$<CONFIG>/bin
    )
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_link_libraries(${PROJECT_NAME} PUBLIC arxApi)

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "Demo"
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".zrx"
)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_COMMAND ${ZWCAD_INSTALL_PATH}/ZwCAD.exe
        VS_DEBUGGER_WORKING_DIRECTORY ${ZWCAD_INSTALL_PATH}
    )
endif()
