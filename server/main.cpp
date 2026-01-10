#include "translate_server.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <string>
#include <format>
#include <memory>
#include <iostream>

int main()
{
    constexpr uint16_t port = 50051;

    std::string server_address = std::format("0.0.0.0:{}", port);

    translate_server service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr server(builder.BuildAndStart());
    std::cout << "Listening on: " << server_address << std::endl;

    server->Wait();

    return 0;
}