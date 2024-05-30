#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <unistd.h>
#include <future>
#include <chrono>

#include <curl/curl.h>
using namespace std::chrono;

struct Result {
    bool success;
    int64_t latencyUsecs;
};

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

Result launchGetRequest(const std::string& address) {
    // todo: object pool to reuse handles. 
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

// todo: add in arg parsing
// usage: ./bench <address> --qps <num> [--duration <secs>] [--numhandles <handles>] [--warmupDuration <secs>]
int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // options
    const std::string address = "http://www.google.com"; //todo: make this an arg
    const int duration = 5;
    const int qps = 25;
    const int64_t usecs = (1e6 / qps);

    std::vector<std::future<Result>> futures;
    futures.reserve(qps * duration);
    auto start = high_resolution_clock::now();
    for (int i = 0; i < qps*duration; ++i) {
        auto lastClock = high_resolution_clock::now();
        // launch request asynchronously
        futures.push_back(std::async(std::launch::async, launchGetRequest, address));

        // figure out how much time is left before submitting the next request
        auto usecsSinceLast = duration_cast<microseconds>(high_resolution_clock::now() - lastClock).count();
        auto usecsLeft = usecs - usecsSinceLast;
        usleep(usecsLeft);
    }
    auto submitStop = high_resolution_clock::now();

    // wait on futures and get results
    int successes = 0;
    int64_t latencySum = 0;
    for (int i = 0; i < futures.size(); ++i) {
        auto &f = futures[i];
        Result result = f.get();
        successes += (int)result.success;
        latencySum += result.latencyUsecs;
    }
    auto stop = high_resolution_clock::now();

    std::cout << "Submitted " << futures.size() << " requests over " 
            << duration_cast<milliseconds>(submitStop - start).count() / 1000.0 << " seconds at " << qps << " qps\n";
    std::cout << "Success rate: " << 100 * (double)successes / futures.size() << "%\n";
    std::cout << "Avg request latency: " << (double)latencySum / futures.size() / 1000.0 << "ms\n";
    std::cout << "Total submit + receive time: " << duration_cast<milliseconds>(stop - start).count() / 1000.0 << " seconds\n";

    curl_global_cleanup();
    return 0;
}
