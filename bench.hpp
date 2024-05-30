#include <vector>
#include <string>

struct Options {
    std::string address;
    int qps;
    int duration;
    int warmup;
};

struct Result {
    bool success;
    int64_t latencyUsecs;
};

struct BenchmarkStats {
    int64_t totalRequests;
    double successRate;
    int64_t avgLatencyMs;
    int64_t submissionTimeMs;
    int64_t waitTimeMs;
    std::vector<Result> results;
};

BenchmarkStats RunHttpBenchmark(Options options);
