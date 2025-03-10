project(arxApi)

file(GLOB_RECURSE INCS CONFIGURE_DEPENDS inc/*.h)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.cpp src/*.def)
file(GLOB_RECURSE LIBS CONFIGURE_DEPENDS ${zrxsdk_dir}/lib-${CMAKE_VS_PLATFORM_NAME}/*.lib)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS} ${INCS})

add_library(${PROJECT_NAME} SHARED ${INCS} ${SRCS})

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:wchar_t /MD>
    $<$<CXX_COMPILER_ID:GNU>:-Wall>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
target_precompile_headers(${PROJECT_NAME} PUBLIC inc/stdafx.h)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _AFXDLL _AFXEXT _WIN32_WINNT=0x0600 KK_OBJECTARX)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${zrxsdk_dir}/inc
    ${zrxsdk_dir}/arxport/inc
    ${zrxsdk_dir}/arxport/inc-x64
    inc
)

target_link_directories(${PROJECT_NAME} PUBLIC ${zrxsdk_dir}/lib-${CMAKE_VS_PLATFORM_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "Api"
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".zrx"

    # RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin
    # ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib
    # LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin
)

if(MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_COMMAND ${ZWCAD_INSTALL_PATH}/ZwCAD.exe
        VS_DEBUGGER_WORKING_DIRECTORY ${ZWCAD_INSTALL_PATH}
    )
endif()