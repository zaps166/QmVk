cmake_minimum_required(VERSION 3.1)
project(QmVk)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

if(QMVK_FIND_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

if(NOT QMVK_GLSLC_COMMAND)
    set(QMVK_GLSLC_COMMAND "glslc" CACHE INTERNAL "")
endif()

if(NOT QMVK_LIBRARY_TYPE)
    set(QMVK_LIBRARY_TYPE OBJECT)
endif()

macro(qmvk_add_shader SHADER_SOURCE_FILE)
    set(EXTRA_ARGS ${ARGN})
    list(LENGTH EXTRA_ARGS NUM_EXTRA_ARGS)
    if(${NUM_EXTRA_ARGS} EQUAL 1)
        list(GET EXTRA_ARGS 0 SHADER_DEPENDENCIES)
    elseif(${NUM_EXTRA_ARGS} GREATER 1)
        message(FATAL_ERROR "Invalid argument count")
    endif()
    unset(NUM_EXTRA_ARGS)
    unset(EXTRA_ARGS)

    get_filename_component(SHADER_SOURCE_FILE_NAME ${SHADER_SOURCE_FILE} NAME)

    get_filename_component(SHADER_SOURCE_STAGE ${SHADER_SOURCE_FILE} LAST_EXT)
    string(TOLOWER ${SHADER_SOURCE_STAGE} SHADER_SOURCE_STAGE)
    if(SHADER_SOURCE_STAGE MATCHES ".glsl")
        get_filename_component(SHADER_SOURCE_STAGE ${SHADER_SOURCE_FILE} NAME_WLE)
        get_filename_component(SHADER_SOURCE_STAGE ${SHADER_SOURCE_STAGE} LAST_EXT)
        string(TOLOWER ${SHADER_SOURCE_STAGE} SHADER_SOURCE_STAGE)
    endif()
    string(SUBSTRING ${SHADER_SOURCE_STAGE} 1 -1 SHADER_SOURCE_STAGE)

    set(SHADER_COMPILED_FILES_DIR "${CMAKE_CURRENT_BINARY_DIR}/qmvk_shaders")
    set(SHADER_COMPILED_FILE_NAME "${SHADER_SOURCE_FILE_NAME}.spv")
    set(SHADER_COMPILED_FILE "${SHADER_COMPILED_FILES_DIR}/${SHADER_COMPILED_FILE_NAME}")

    add_custom_command(
        OUTPUT ${SHADER_COMPILED_FILE}
        DEPENDS ${SHADER_SOURCE_FILE} ${SHADER_DEPENDENCIES}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_COMPILED_FILES_DIR}
        COMMAND ${QMVK_GLSLC_COMMAND} -std=450 -fshader-stage=${SHADER_SOURCE_STAGE} -o ${SHADER_COMPILED_FILE} ${SHADER_SOURCE_FILE}
    )

    list(APPEND QMVK_SHADER_COMPILED_FILES ${SHADER_COMPILED_FILE})

    unset(SHADER_COMPILED_FILE)
    unset(SHADER_COMPILED_FILE_NAME)
    unset(SHADER_COMPILED_FILES_DIR)
    unset(SHADER_SOURCE_STAGE)
    unset(SHADER_SOURCE_FILE_NAME)

    unset(SHADER_DEPENDENCIES)
endmacro()

file(GLOB QMVK_VULKAN_HDR
    "*.hpp"
)
file(GLOB QMVK_VULKAN_SRC
    "*.cpp"
)

add_library(${PROJECT_NAME} ${QMVK_LIBRARY_TYPE}
    ${QMVK_VULKAN_HDR}
    ${QMVK_VULKAN_SRC}
)

if(QMVK_USE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${QMVK_VULKAN_HDR}
    )
endif()

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    -DVK_NO_PROTOTYPES
    -DVULKAN_HPP_TYPESAFE_CONVERSION
    -DNOMINMAX
)
if(QMVK_USE_IMAGE_BUFFER_VIEW)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        -DQMVK_USE_IMAGE_BUFFER_VIEW
    )
endif()

if(QMVK_NO_EXPORT)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        -DQMVK_NO_EXPORT
    )
endif()

if(QMVK_WAIT_TIMEOUT_MS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        -DQMVK_WAIT_TIMEOUT_MS=${QMVK_WAIT_TIMEOUT_MS}
    )
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        -DVK_USE_PLATFORM_WIN32_KHR
        PRIVATE
        -DQMVK_LIBRARY
    )
elseif(ANDROID)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        -DVK_USE_PLATFORM_ANDROID_KHR
    )
endif()
