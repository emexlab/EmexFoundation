cmake_minimum_required(VERSION 3.16)

project(EmexFoundation
    VERSION 1.0.0
    LANGUAGES C
)

find_package(Threads REQUIRED)

add_library(EmexFoundation STATIC
    src/EFRuntime/EFBase.c
    src/EFRuntime/EFAllocator.c

    src/EFNumber.c
    src/EFString.c
    src/EFArray.c
    src/EFPage.c
    src/EFPageGroup.c
    src/EFData.c
    src/EFFileHandle.c
    src/EFProcess.c
    src/EFURL.c
    src/EFFile.c
    src/EFBitWalker.c
)

target_include_directories(EmexFoundation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(EmexFoundation PUBLIC c_std_23)

set_target_properties(EmexFoundation PROPERTIES
    C_EXTENSIONS ON
)

if (MSVC)
    # Fuck MSVC nobody likes you
    message(FATAL_ERROR "MSVC is not supported. Switch to Linux moron, microslop sheep!")
else()

    # Setting up compiler flags
    set(_EF_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-gnu-conditional-omitted-operand)
    set(_EF_OPT_FLAGS -fPIC)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND _EF_OPT_FLAGS -O0 -g)
    else()
        list(APPEND _EF_OPT_FLAGS -O3 -fomit-frame-pointer -fstrict-aliasing -funroll-loops)

        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            list(APPEND _EF_OPT_FLAGS -fgraphite-identity -floop-nest-optimize -fipa-pta)
        elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
            list(APPEND _EF_OPT_FLAGS -fvectorize -fslp-vectorize)
        endif()
    endif()

    target_compile_definitions(EmexFoundation PRIVATE $<$<CONFIG:Debug>:DEBUG>)
    target_compile_options(EmexFoundation PRIVATE ${_EF_WARN_FLAGS} ${_EF_OPT_FLAGS})
endif()

target_link_libraries(EmexFoundation
    PRIVATE
        Threads::Threads
)

install(TARGETS EmexFoundation
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.h"
)
