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

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

get_target_property(gRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)

function(proto_gen PROTO_FILE SRC HDR)
    get_filename_component(FILE_NAME ${PROTO_FILE} NAME_WE)

    set(_pb_cc "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_NAME}.pb.cc")
    set(_pb_h "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_NAME}.pb.h")
    set(_grpc_pb_cc "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_NAME}.grpc.pb.cc")
    set(_grpc_pb_h "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_NAME}.grpc.pb.h")

    add_custom_command(
            OUTPUT "${_pb_cc}" "${_pb_h}" "${_grpc_pb_cc}" "${_grpc_pb_h}"
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ARGS --grpc_out=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}
            --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN}
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE} protobuf::protoc gRPC::grpc_cpp_plugin
            COMMENT "compiling proto"
            VERBATIM
    )

    set(${SRC} "${_pb_cc}" "${_grpc_pb_cc}" PARENT_SCOPE)
    set(${HDR} "${_pb_h}" "${_grpc_pb_h}" PARENT_SCOPE)
endfunction()

set(PROTO_SRC)
set(PROTO_HDR)

proto_gen(server/translate.proto PROTO_SRC PROTO_HDR)

add_executable(translate
        engine/translate_engine.h
        engine/translate_engine.cpp
        server/translate_server.h
        ${PROTO_SRC} ${PROTO_HDR}
)
target_include_directories(translate PRIVATE engine)
target_link_libraries(translate PRIVATE
        llama
        gRPC::grpc++
        gRPC::grpc++_reflection
        protobuf::libprotobuf
)