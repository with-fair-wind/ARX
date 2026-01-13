project(arxApi)

# Determine target SDK platform directory suffix (works for Ninja or MSVC generators)
# Prefer CMAKE_VS_PLATFORM_NAME if provided (Visual Studio), otherwise detect pointer size.
if(DEFINED CMAKE_VS_PLATFORM_NAME AND NOT CMAKE_VS_PLATFORM_NAME STREQUAL "")
    set(_ZRXSDK_ARCH "${CMAKE_VS_PLATFORM_NAME}")
else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_ZRXSDK_ARCH "x64")
    else()
        set(_ZRXSDK_ARCH "x86")
    endif()
endif()

# File lists
file(GLOB_RECURSE INCS CONFIGURE_DEPENDS inc/*.h)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.cpp src/*.def)
file(GLOB_RECURSE LIBS ${zrxsdk_dir}/lib-${_ZRXSDK_ARCH}/*.lib)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS} ${INCS})

add_library(${PROJECT_NAME} SHARED ${INCS} ${SRCS})

target_compile_options(${PROJECT_NAME} PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:wchar_t /MD /utf-8>
    $<$<CXX_COMPILER_ID:GNU>:-Wall>
)
target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE _UNICODE)
target_precompile_headers(${PROJECT_NAME} PUBLIC inc/stdafx.h)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _AFXDLL _AFXEXT KT_EXPORTARX)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${zrxsdk_dir}/inc>
    $<BUILD_INTERFACE:${zrxsdk_dir}/arxport/inc>
    $<BUILD_INTERFACE:${zrxsdk_dir}/arxport/inc-x64>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    PUBLIC
    $<INSTALL_INTERFACE:include>
)

# 链接目录与库仅在构建时使用，避免把源码目录前缀的绝对路径导出到 INTERFACE 属性
target_link_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${zrxsdk_dir}/lib-${_ZRXSDK_ARCH}>)
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

set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(
    EXPORT ${PROJECT_NAME}
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE KT:: # 目标命名空间（如 MyLibrary::MyLibrary）
    DESTINATION lib/cmake/${PROJECT_NAME}
)

install(
    DIRECTORY inc/ # 源目录
    DESTINATION include # 目标安装目录
    FILES_MATCHING # 只安装匹配的文件
    PATTERN "*.h" # 只安装头文件
    PATTERN "*.hpp" # 也可以安装 .hpp 文件（如果有）
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION 2.0.0
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
)

install(
    FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake DESTINATION lib/cmake/${PROJECT_NAME}
)