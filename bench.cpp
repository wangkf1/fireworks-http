#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <unistd.h>
#include <future>
#include <chrono>

#include <curl/curl.h>

struct Result {
    bool success;
    int64_t latencyUsecs;
};

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

Result launchGetRequest(const std::string& address) {
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    auto start = std::chrono::high_resolution_clock::now();
    auto ret = curl_easy_perform(curl);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    Result result{.success = ret == CURLE_OK, .latencyUsecs = duration.count()};
    return result;
}

// usage: ./bench <address> --qps <num> [--duration <secs>] [--numhandles <handles>] []
int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    constexpr int NUMHANDLES = 10; //todo: make this an arg
    const std::string address = "http://www.google.com"; //todo: make this an arg
    const int duration = 5;
    const int qps = 10;
    const size_t usecs = (1e6 / qps);


    std::vector<std::future<Result>> futures;
    futures.reserve(qps * duration);
    for (int i = 0; i < qps*duration; ++i) {
        futures.push_back(std::async(std::launch::async, launchGetRequest, address));
        usleep(usecs);
    }

    int successes = 0;
    int64_t latencySum = 0;
    for (int i = 0; i < futures.size(); ++i) {
        auto &f = futures[i];
        Result result = f.get();
        successes += (int)result.success;
        latencySum += result.latencyUsecs;
    }

    std::cout << "Success rate: " << 100 * (double)successes / futures.size() << "%\n";
    std::cout << "Avg latency: " << (double)latencySum / futures.size() / 1000.0 << "ms\n";

    curl_global_cleanup();
    return 0;
}
