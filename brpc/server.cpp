
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include "brpc.pb.h"

DEFINE_bool(echo_attachment, false, "Echo attachment as well");
DEFINE_int32(port, 8002, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");
DEFINE_int32(internal_port, -1, "Only allow builtin services at this port");

namespace brpc 
{
    // Your implementation of EchoService
    class EchoServiceImpl : public EchoService 
    {
    public:
        EchoServiceImpl() = default;
        ~EchoServiceImpl() = default;
        void Echo(google::protobuf::RpcController* cntl_base,
                  const EchoReq* request,
                  EchoRes* response,
                  google::protobuf::Closure* done) 
        {
            brpc::ClosureGuard done_guard(done);
            brpc::Controller* cntl =
                static_cast<brpc::Controller*>(cntl_base);

            // Echo request and its attachment
            response->set_message(request->message());
            if (FLAGS_echo_attachment) 
            {
                cntl->response_attachment().append(cntl->request_attachment());
            }
        }
    };
}  // namespace brpc

DEFINE_bool(h, false, "print help information");

int main(int argc, char** argv) 
{
    std::string help_str = "dummy help infomation";
    GFLAGS_NS::SetUsageMessage(help_str);

    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_h) 
    {
        fprintf(stderr, "%s\n%s\n%s", help_str.c_str(), help_str.c_str(), help_str.c_str());
        return 0;
    }

    // Generally you only need one Server.
    brpc::Server server;

    // Instance of your service.
    brpc::EchoServiceImpl echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) 
    {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server. 
    brpc::ServerOptions options;
    // options.mutable_ssl_options()->default_cert.certificate = "cert.pem";
    // options.mutable_ssl_options()->default_cert.private_key = "key.pem";
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.max_concurrency = FLAGS_max_concurrency;
    options.internal_port = FLAGS_internal_port;
    if (server.Start(FLAGS_port, &options) != 0) 
    {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
