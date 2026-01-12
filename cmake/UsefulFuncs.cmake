function(add_executable_with_pch target_name)
    set(options) # 开关选项
    set(oneValueArgs PCH) # 单值选项
    set(multiValueArgs SRCS) # 多值选项

    # 解析输入参数。解析结果都会放到 PARSED_<...> 里
    cmake_parse_arguments(
        PARSED # 前缀：所有结果变量都以 PARSED_ 开头
        "${options}" # 开关选项列表
        "${oneValueArgs}" # 单值选项列表
        "${multiValueArgs}" # 多值选项列表
        ${ARGN} # 函数收到的所有参数
    )
    add_executable(${target_name} ${PARSED_SRCS})
    target_include_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/inc)

    if(PARSED_PCH)
        target_precompile_headers(${target_name} PRIVATE ${PARSED_PCH})
    else()
        target_precompile_headers(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/inc/pch.h)
    endif()

    target_compile_options(${target_name} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
        $<$<CXX_COMPILER_ID:GNU>:-Wall>
    )
endfunction()

function(link_third_party_libraries target_name)
    # 用法: link_third_party_libraries(my_target libA libB ...)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' 不存在！")
    endif()

    foreach(lib IN LISTS ARGN)
        target_link_libraries(${target_name} PRIVATE ${lib})
    endforeach()
endfunction()

function(add_generate_compile_commands_target PS_VERSION)
    if(${PS_VERSION} EQUAL 7)
        set(PS_NAME pwsh.exe)
    else()
        set(PS_NAME powershell.exe)
    endif()

    find_program(POWERSHELL_EXE
        NAMES ${PS_NAME}
        HINTS
        "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0"
        ENV PATH
    )

    if(NOT POWERSHELL_EXE)
        message(FATAL_ERROR "PowerShell (${PS_NAME}) not found. Please make sure it is installed.")
    else()
        message(STATUS "Found PowerShell: ${POWERSHELL_EXE}")
    endif()

    add_custom_target(generate_compile_commands
        COMMENT "Automatically generate compile_commands.json"
        COMMAND ${CMAKE_COMMAND} -E echo "▶ Calling PowerShell script…"
        COMMAND ${POWERSHELL_EXE}
        -NoProfile
        -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/generate_msvc_compile.ps1"
        -Config $<CONFIG>
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endfunction()