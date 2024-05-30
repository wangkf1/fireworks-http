#include <vector>
#include <string>

struct Options {
    std::string address;    // http address for all requests
    int qps;                // queries per second
    int duration;           // duration of test in seconds
    int warmup;             // warmup in seconds
};

struct Result {
    bool success;
    int64_t latencyUsecs;   // round trip latency of request+receive
};

struct BenchmarkStats {
    int64_t totalRequests;  // total requests made (qps * duration)
    double successRate;     // fraction of requests that returned success
    int64_t avgLatencyMs;   // average roundtrip latency of all http req+recv
    int64_t submissionTimeMs;// wall time from start to finish of submissions
    int64_t waitTimeMs;     // time spent waiting on remaining requests 
    std::vector<Result> results;
};

BenchmarkStats RunHttpBenchmark(Options options);
