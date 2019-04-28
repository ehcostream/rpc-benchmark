#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <sstream>

#include <grpcpp/grpcpp.h>
#include <gflags/gflags.h>

#ifdef BAZEL_BUILD
#include "examples/protos/test.grpc.pb.h"
#else
#include "test.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
using test::EchoReq;
using test::EchoRes;
using test::EchoService;


DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_int32(request_size, 16, "Bytes of each request");
DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_int32(concurrency_count, 1000000, "Number of invoking the service");


std::string g_strRequest;


class TestClient {
 public:
  TestClient(std::shared_ptr<Channel> channel)
      : stub_(EchoService::NewStub(channel)) {}
  void Echo() {
    EchoReq request;
    request.set_message(g_strRequest);
    EchoRes reply;
    ClientContext context;
    Status status = stub_->Echo(&context, request, &reply);
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }

 private:
  std::unique_ptr<EchoService::Stub> stub_;
};

static void* sender(void* arg)
{
    TestClient* client = static_cast<TestClient*>(arg);
    int32_t cnt = FLAGS_concurrency_count / FLAGS_thread_num;
    while (cnt--) 
    {
        client->Echo();
    }
}

int main(int argc, char** argv) 
{

    google::ParseCommandLineFlags(&argc, &argv, true);
    g_strRequest.resize(FLAGS_request_size);
    //record the start time 
    auto start = std::chrono::system_clock().now();

    TestClient client(grpc::CreateChannel(
      FLAGS_server, grpc::InsecureChannelCredentials()));

    int nError = 0;
    std::ostringstream oss;
    do
    {
        std::vector<pthread_t> pids;
        pids.resize(FLAGS_thread_num);
        //assign the task num for per thread
        for(int i = 0; i < FLAGS_thread_num; ++i)
        {
            if(pthread_create(&pids[i], nullptr, sender, &client) != 0)
            {
             oss << "Fail to create thread";
             nError = 3;
             break;
            }
        }

        std::cout << "create thread Successfully" << std::endl;
        //main thread will not exist until all child threads finished their.
        for(int i = 0; i < FLAGS_thread_num; ++i)
        {
            pthread_join(pids[i], nullptr);

        }
        std::cout << "join finished" << std::endl;

    }while(false);
    if(nError != 0)
    {
        std::cout << "Error code=" << nError << "," << oss.str() << std::endl;
    }
    else
    {
        std::cout << "Successfully" << std::endl;
    }

    
    //record the end time
    auto end = std::chrono::system_clock().now();
    auto elaplsedTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "elapsed time is " << elaplsedTime.count() << "us" << std::endl;
    std::cout << "qps:" << (double)FLAGS_concurrency_count / ((double)elaplsedTime.count() / (1000 * 1000)) << std::endl;

    return 0;
}
