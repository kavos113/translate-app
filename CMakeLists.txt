cmake_minimum_required(VERSION 3.29)
project(translate CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/Zc:char8_t-)
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(GGML_VULKAN ON CACHE BOOL "Build with vulkan" FORCE)

option(LLAMA_BUILD_TESTS "Build tests" OFF)
option(LLAMA_BUILD_EXAMPLES "Build examples" OFF)
option(LLAMA_BUILD_SERVER "Build server" OFF)

add_subdirectory(llama.cpp)

add_executable(translate
        engine/translate_engine.h
        engine/translate_engine.cpp
)
target_link_libraries(translate PRIVATE llama)