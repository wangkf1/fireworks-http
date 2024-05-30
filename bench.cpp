#include <cassert>
#include <unistd.h>
#include <future>
#include <chrono>

#include <curl/curl.h>

#include "bench.hpp"

using namespace std::chrono;

// do nothing with the returned data
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

// launch function passed to async
Result launchGetRequest(const std::string& address) {
    // todo: object pool to reuse handles. 
    // todo: macro to check errors and exit early with failure
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // perform request, measuring latency
    auto start = high_resolution_clock::now();
    auto ret = curl_easy_perform(curl);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    // return w/ success and latency
    Result result{.success = ret == CURLE_OK, .latencyUsecs = duration.count()};
    return result;
}

BenchmarkStats RunHttpBenchmark(Options options) {
    // setup
    curl_global_init(CURL_GLOBAL_DEFAULT);
    const int64_t usecs = (1e6 / options.qps);
    std::vector<std::future<Result>> futures;
    futures.reserve(options.qps * options.duration);

    // warmup requests (useful for when we have threadpools and object pools to build up)
    for (int i = 0; i < options.qps*options.warmup; ++i) {
        // launch request asynchronously
        futures.push_back(std::async(std::launch::async, launchGetRequest, options.address));
        usleep(usecs);
    }
    for (auto &f : futures) {
        f.wait();
    }
    futures.clear();
    
    // submit requests at a fixed rate
    auto start = high_resolution_clock::now();
    for (int i = 0; i < options.qps*options.duration; ++i) {
        auto lastClock = high_resolution_clock::now();
        // launch request asynchronously
        futures.push_back(std::async(std::launch::async, launchGetRequest, options.address));

        // figure out how much time is left before submitting the next request
        auto usecsSinceLast = duration_cast<microseconds>(high_resolution_clock::now() - lastClock).count();
        auto usecsLeft = usecs - usecsSinceLast;
        usleep(usecsLeft);
    }
    auto submitStop = high_resolution_clock::now();

    // wait on futures and get results
    int successes = 0;
    int64_t latencySum = 0;
    BenchmarkStats stats;
    stats.results.reserve(options.qps * options.duration);
    for (int i = 0; i < futures.size(); ++i) {
        auto &f = futures[i];
        Result result = f.get();
        successes += (int)result.success;
        latencySum += result.latencyUsecs;
        stats.results.push_back(result);
    }
    auto stop = high_resolution_clock::now();
    curl_global_cleanup();

    // calculate stats
    stats.submissionTimeMs = duration_cast<milliseconds>(submitStop - start).count();
    stats.waitTimeMs = duration_cast<milliseconds>(stop - submitStop).count();
    stats.successRate = (double)successes / futures.size();
    stats.avgLatencyMs = (double)latencySum / futures.size() / 1000.0;
    return stats;
}
