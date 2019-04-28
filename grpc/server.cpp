#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <gflags/gflags.h>

#ifdef BAZEL_BUILD
#include "examples/protos/test.grpc.pb.h"
#else
#include "test.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::EchoReq;
using test::EchoRes;
using test::EchoService;

DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");

// Logic and data behind the server's behavior.
class EchoServiceImpl final : public EchoService::Service 
{
    Status Echo(ServerContext* context, const EchoReq* request,
                    EchoRes* reply) override 
    {
      reply->set_message(request->message());
      return Status::OK;
    }
};

void RunServer() 
{
    std::string server_address(FLAGS_server);
    EchoServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) 
{
    RunServer();

    return 0;
}
