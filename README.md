# Test Object

* [brpc]("https://github.com/apache/incubator-brpc")
* [grpc]("https://github.com/grpc/grpc")

## Standard

1. single channel
2. shutdown load balance
3. create the thread with 'std::thread'
4. use the same pb package data,ignore the effect as the difference of protobuf(grpc:v3.7.1,brpc:v3.6.1)
5. thread cout is  50
6. concurrency count is 1,000,000

## Indicator
* qps
* latency time
