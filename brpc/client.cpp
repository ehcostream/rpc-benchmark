#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include <brpc/server.h>
#include <bvar/bvar.h>
#include <sstream>
#include <chrono>
#include <thread>

#include "brpc.pb.h"

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_int32(attachment_size, 0, "Carry so many byte attachment along with requests");
DEFINE_int32(request_size, 16, "Bytes of each request");
DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_bool(enable_ssl, false, "Use SSL connection");
DEFINE_int32(dummy_port, 6666, "Launch dummy server at this port");
DEFINE_int32(concurrency_count, 1000000, "Number of invoking the service");

std::string g_strRequest;

bvar::LatencyRecorder g_latency_recorder("client");
bvar::Adder<int> g_error_count("client_error_count");

static void* sender(void* arg)
{
    brpc::EchoService_Stub stub(static_cast<google::protobuf::RpcChannel*>(arg));

    int log_id = 0;
    int32_t cnt = FLAGS_concurrency_count / FLAGS_thread_num;
    while (cnt-- && !brpc::IsAskedToQuit()) 
    {
        brpc::EchoReq request;
        brpc::EchoRes response;
        brpc::Controller cntl;

        request.set_message(g_strRequest);
        cntl.set_log_id(log_id++);  // set by user

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        stub.Echo(&cntl, &request, &response, NULL);
        if (!cntl.Failed()) 
        {
            g_latency_recorder << cntl.latency_us();
        } 
        else 
        {
            g_error_count << 1; 
            CHECK(brpc::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            //bthread_usleep(50000);
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    //record the start time 
    auto start = std::chrono::system_clock().now();

    int nError = 0;
    std::ostringstream oss;
    do
    {
        //initialize the channel option
        brpc::ChannelOptions options;
        if(FLAGS_enable_ssl)
        {
            options.mutable_ssl_options();
        }
        options.protocol = FLAGS_protocol;
        options.connection_type = FLAGS_connection_type;
        options.connect_timeout_ms = std::min(FLAGS_timeout_ms / 2 , 100);
        options.max_retry = FLAGS_max_retry;

        brpc::Channel channel;
        //initialize the channel
        if(channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0)
        {
            oss << "Fail to initialize channel";
            nError = 1;
            break;
        }
        if(FLAGS_request_size <= 0)
        {
            oss << "Bad request size=" << FLAGS_request_size;
            nError = 2;
            break;
        }

        g_strRequest.resize(FLAGS_request_size, 'r');

        //set the hook port for the web server 
        if(FLAGS_dummy_port >= 0)
        {
            brpc::StartDummyServerAt(FLAGS_dummy_port);
        }
        //std::vector<bthread_t> bids;
        std::vector<pthread_t> pids;
        if(!FLAGS_use_bthread)
        {
            pids.resize(FLAGS_thread_num);
            //assign the task num for per thread
            //int32_t nCCnt = FLAGS_concurrency_count / FLAGS_thread_num;
            for(int i = 0; i < FLAGS_thread_num; ++i)
            {
                if(pthread_create(&pids[i], nullptr, sender, &channel) != 0)
                {
                 oss << "Fail to create thread";
                 nError = 3;
                 break;
                }
            }
        }
        else
        {
            //do not handle
        }

        std::cout << "create thread Successfully" << std::endl;
        //main thread will not exist until all child threads finished their.
        for(int i = 0; i < FLAGS_thread_num; ++i)
        {
            if(!FLAGS_use_bthread)
            {
                pthread_join(pids[i], nullptr);
            }
            else
            {
                //do no handle
            }
        }

    }while(false);

    if(nError != 0)
    {
        LOG(ERROR) << "Error code=" << nError << "," << oss.str();
    }
    else
    {
        LOG(DEBUG) << "Successfully";
    }

    
    //record the end time
    auto end = std::chrono::system_clock().now();
    auto elaplsedTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "elapsed time is " << elaplsedTime.count() << "us" << std::endl;
    std::cout << "qps:" << (double)FLAGS_concurrency_count / ((double)elaplsedTime.count() / (1000 * 1000)) << std::endl;
    
}